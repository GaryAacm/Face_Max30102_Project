package main

import (
	"bytes"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/dgrijalva/jwt-go"
	"github.com/gin-gonic/gin"
	"golang.org/x/crypto/bcrypt"
	"gopkg.in/yaml.v2"
	"gorm.io/driver/mysql"
	"gorm.io/gorm"
)

// 定义User模型
type User struct {
	ID        uint   `gorm:"primaryKey"`
	IDNumber  string `gorm:"unique"`
	Password  string
	Age       *int
	Gender    *string
	Role      string
	CreatedAt time.Time      // 记录创建时间
	UpdatedAt time.Time      // 记录最后更新时间
	DeletedAt gorm.DeletedAt `gorm:"index"` // 软删除时间（可选）
}

// 定义SensorData模型
type SensorData struct {
	ID        uint      `gorm:"primarykey"`
	UserID    uint      `gorm:"index"`    // 关联用户ID
	Timestamp time.Time `gorm:"index"`    // 数据时间戳
	IRSignal  int       `gorm:"not null"` // IR 信号
	RedSignal int       `gorm:"not null"` // 红光信号
	BatchID   string    `gorm:"not null"` // 采集批次ID
	CreatedAt time.Time // 记录创建时间
}

// 配置结构体
type Config struct {
	Server struct {
		Address string `yaml:"address"`
		Model   string `yaml:"model"`
	} `yaml:"server"`
	DB struct {
		Dialect  string `yaml:"dialects"`
		Host     string `yaml:"host"`
		Port     int    `yaml:"port"`
		Db       string `yaml:"db"`
		Username string `yaml:"username"`
		Password string `yaml:"password"`
		Charset  string `yaml:"charset"`
		MaxIdle  int    `yaml:"maxIdle"`
		MaxOpen  int    `yaml:"maxOpen"`
	} `yaml:"db"`
}

var DB *gorm.DB
var AppConfig *Config

// JWT key used to create the signature
var jwtKey = []byte("your_secret_key")

// Claims struct
type Claims struct {
	UserID uint   `json:"user_id"`
	Role   string `json:"role"`
	jwt.StandardClaims
}

// 加载配置文件
func LoadConfig() *Config {
	config := &Config{}
	yamlFile, err := ioutil.ReadFile("config.yaml")
	if err != nil {
		log.Fatalf("yamlFile.Get err #%v ", err)
	}
	err = yaml.Unmarshal(yamlFile, config)
	if err != nil {
		log.Fatalf("Unmarshal: %v", err)
	}
	return config
}

// 初始化数据库
func initDB(config *Config) *gorm.DB {
	dsn := fmt.Sprintf("%s:%s@tcp(%s:%d)/%s?charset=%s&parseTime=True&loc=Local",
		config.DB.Username, config.DB.Password, config.DB.Host, config.DB.Port, config.DB.Db, config.DB.Charset)
	db, err := gorm.Open(mysql.Open(dsn), &gorm.Config{})
	if err != nil {
		panic("failed to connect database")
	}
	return db
}

// InitConfig 初始化配置，包括数据库
func InitConfig() {
	AppConfig = LoadConfig()
	DB = initDB(AppConfig)
}

// JWT生成函数
func generateJWT(user User) (string, error) {
	expirationTime := time.Now().Add(24 * time.Hour)
	claims := &Claims{
		UserID: user.ID,
		Role:   user.Role,
		StandardClaims: jwt.StandardClaims{
			ExpiresAt: expirationTime.Unix(),
		},
	}
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	tokenString, err := token.SignedString(jwtKey)
	if err != nil {
		return "", err
	}
	return tokenString, nil
}

func authMiddleware() gin.HandlerFunc {
	return func(c *gin.Context) {
		authHeader := c.GetHeader("Authorization")
		if authHeader == "" {
			c.JSON(http.StatusUnauthorized, gin.H{"code": 401, "message": "请求头中缺少Authorization"})
			c.Abort()
			return
		}

		tokenString := strings.TrimPrefix(authHeader, "Bearer ")
		if tokenString == authHeader {
			c.JSON(http.StatusUnauthorized, gin.H{"code": 401, "message": "Authorization格式错误"})
			c.Abort()
			return
		}

		claims := &Claims{}
		token, err := jwt.ParseWithClaims(tokenString, claims, func(token *jwt.Token) (interface{}, error) {
			return jwtKey, nil
		})

		if err != nil || !token.Valid {
			c.JSON(http.StatusUnauthorized, gin.H{"code": 401, "message": "无效的Token"})
			c.Abort()
			return
		}

		c.Set("user_id", claims.UserID)
		c.Set("role", claims.Role)
		c.Next()
	}
}

func main() {
	// 初始化配置、数据库
	InitConfig()

	// 自动迁移User和SensorData模型
	DB.AutoMigrate(&User{}, &SensorData{})

	r := gin.Default()
	r.Use(func(c *gin.Context) {
		c.Writer.Header().Set("Access-Control-Allow-Origin", "*") // 或者明确设置为允许的域名
		c.Writer.Header().Set("Access-Control-Allow-Credentials", "true")
		c.Writer.Header().Set("Access-Control-Allow-Headers", "Content-Type, Content-Length, Authorization, X-Requested-With")
		c.Writer.Header().Set("Access-Control-Allow-Methods", "POST, OPTIONS, GET, PUT, DELETE")

		if c.Request.Method == "OPTIONS" {
			c.AbortWithStatus(204)
			return
		}

		c.Next()
	})

	// 注册接口
	r.POST("/register", func(ctx *gin.Context) {
		var req struct {
			IDNumber string `json:"id_number"`
			Password string `json:"password"`
			Role     string `json:"role"`
		}

		if err := ctx.ShouldBindJSON(&req); err != nil {
			ctx.JSON(http.StatusBadRequest, gin.H{"error": "Invalid request format"})
			return
		}

		idNumber := req.IDNumber
		password := req.Password
		role := req.Role

		// 数据验证
		if len(idNumber) == 0 {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "ID号不能为空",
			})
			return
		}
		if len(password) < 6 {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "密码不能少于6位",
			})
			return
		}
		if role != "user" && role != "admin" {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "无效的用户角色",
			})
			return
		}

		// 判断ID号是否存在
		var user User
		result := DB.Where("id_number = ?", idNumber).First(&user)
		if result.RowsAffected > 0 {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "用户已存在",
			})
			return
		}

		// 创建用户
		hashedPassword, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
		if err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "密码加密错误",
			})
			return
		}
		newUser := User{
			IDNumber: idNumber,
			Password: string(hashedPassword),
			Role:     role,
		}

		if err := DB.Create(&newUser).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "用户创建失败",
			})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{
			"code":    200,
			"message": "注册成功",
		})
	})

	// 登录接口
	r.POST("/login", func(ctx *gin.Context) {
		idNumber := ctx.PostForm("id_number")
		password := ctx.PostForm("password")

		// 查询用户
		var user User
		result := DB.Where("id_number = ?", idNumber).First(&user)
		if result.Error != nil {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "用户不存在",
			})
			return
		}

		// 检查密码
		if err := bcrypt.CompareHashAndPassword([]byte(user.Password), []byte(password)); err != nil {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "密码错误",
			})
			return
		}

		// 生成JWT
		tokenString, err := generateJWT(user)
		if err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "Token生成失败",
			})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{
			"code":      200,
			"token":     tokenString,
			"id_number": user.IDNumber, // 这里返回的是id_number
		})
	})

	// 获取用户自己的数据（需要登录）
	r.GET("/mydata", authMiddleware(), func(ctx *gin.Context) {
		userID := ctx.GetUint("user_id") // 从 JWT 中获取 user_id

		var sensorData []SensorData
		if err := DB.Where("user_id = ?", userID).Find(&sensorData).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "数据查询失败",
			})
			return
		}

		if len(sensorData) == 0 {
			ctx.JSON(http.StatusOK, gin.H{
				"code":    200,
				"message": "当前用户尚未存在数据，请扫码",
				"data":    nil,
			})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{
			"code": 200,
			"data": sensorData,
		})
	})

	// 管理员登录接口
	r.POST("/admin/login", func(ctx *gin.Context) {
		idNumber := ctx.PostForm("id_number")
		password := ctx.PostForm("password")

		// 查询管理员
		var admin User
		result := DB.Where("id_number = ? AND role = 'admin'", idNumber).First(&admin)
		if result.Error != nil || admin.Password != password {
			ctx.JSON(http.StatusUnauthorized, gin.H{
				"code":    401,
				"message": "管理员账号或密码错误",
			})
			return
		}

		// 生成JWT
		tokenString, err := generateJWT(admin)
		if err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "Token生成失败",
			})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{
			"code":  200,
			"token": tokenString,
		})
	})

	// 管理员新增用户接口（需要管理员权限）
	r.POST("/admin/users", authMiddleware(), func(ctx *gin.Context) {
		role := ctx.GetString("role")
		if role != "admin" {
			ctx.JSON(http.StatusForbidden, gin.H{"code": 403, "message": "无权限"})
			return
		}

		var req struct {
			IDNumber string  `json:"id_number"`
			Password string  `json:"password"`
			Age      *int    `json:"age"`
			Gender   *string `json:"gender"`
		}

		// 打印接收到的请求体
		bodyBytes, _ := ioutil.ReadAll(ctx.Request.Body)
		fmt.Println("Request Body:", string(bodyBytes))

		// 重新填充请求体，以便后续可以继续使用
		ctx.Request.Body = ioutil.NopCloser(bytes.NewBuffer(bodyBytes))

		if err := ctx.ShouldBindJSON(&req); err != nil {
			fmt.Println("Error Binding JSON:", err)
			ctx.JSON(http.StatusBadRequest, gin.H{"error": "Invalid request format"})
			return
		}
		// 验证账号和密码
		if len(req.IDNumber) == 0 || len(req.Password) < 6 {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "账号不能为空且密码不能少于6位",
			})
			return
		}

		// 检查用户IDNumber是否已经存在
		var existingUser User
		if err := DB.Where("id_number = ?", req.IDNumber).First(&existingUser).Error; err == nil {
			ctx.JSON(http.StatusConflict, gin.H{"message": "用户IDNumber已存在"})
			return
		}

		// 创建新用户
		hashedPassword, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
		if err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"message": "密码加密错误"})
			return
		}

		newUser := User{
			IDNumber: req.IDNumber,
			Password: string(hashedPassword),
			Age:      req.Age,
			Gender:   req.Gender,
			Role:     "user",
		}

		if err := DB.Create(&newUser).Error; err != nil {
			fmt.Println("Error Creating User:", err)
			ctx.JSON(http.StatusInternalServerError, gin.H{"message": "用户创建失败"})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{"message": "用户创建成功"})
	})

	// 管理员获取所有用户信息接口（需要管理员权限）
	r.GET("/admin/users", authMiddleware(), func(ctx *gin.Context) {
		role := ctx.GetString("role")
		log.Println("Role from JWT:", role) // 添加日志以检查角色
		if role != "admin" {
			ctx.JSON(http.StatusForbidden, gin.H{"code": 403, "message": "无权限"})
			return
		}

		var users []User
		if err := DB.Where("role = ?", "user").Find(&users).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "用户信息查询失败",
			})
			return
		}

		// 创建一个只包含需要传递字段的简化用户结构体切片
		simplifiedUsers := []gin.H{}
		for _, user := range users {
			simplifiedUsers = append(simplifiedUsers, gin.H{
				"id":         user.ID,
				"id_number":  user.IDNumber,
				"age":        user.Age,
				"gender":     user.Gender,
				"created_at": user.CreatedAt,
			})
		}

		ctx.JSON(http.StatusOK, gin.H{
			"code": 200,
			"data": simplifiedUsers,
		})
	})

	// 管理员更新用户信息接口（需要管理员权限）
	r.PUT("/admin/users/:id", authMiddleware(), func(ctx *gin.Context) {
		role := ctx.GetString("role")
		if role != "admin" {
			ctx.JSON(http.StatusForbidden, gin.H{"code": 403, "message": "无权限"})
			return
		}

		var req struct {
			IDNumber string  `json:"id_number"`
			Age      *int    `json:"age"`
			Gender   *string `json:"gender"`
		}

		id := ctx.Param("id")

		if err := ctx.ShouldBindJSON(&req); err != nil {
			ctx.JSON(http.StatusBadRequest, gin.H{"error": "Invalid request format"})
			return
		}

		var user User
		if err := DB.Where("id = ?", id).First(&user).Error; err != nil {
			ctx.JSON(http.StatusNotFound, gin.H{"code": 404, "message": "用户未找到"})
			return
		}

		user.IDNumber = req.IDNumber
		user.Age = req.Age
		user.Gender = req.Gender

		if err := DB.Save(&user).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "更新失败"})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{"code": 200, "message": "更新成功"})
	})

	// 管理员删除用户接口（需要管理员权限）
	r.DELETE("/admin/users/:id", authMiddleware(), func(ctx *gin.Context) {
		role := ctx.GetString("role")
		if role != "admin" {
			ctx.JSON(http.StatusForbidden, gin.H{"code": 403, "message": "无权限"})
			return
		}

		id := ctx.Param("id")

		if err := DB.Delete(&User{}, id).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "删除失败"})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{"code": 200, "message": "删除成功"})
	})

	r.Run(":8080") // 在端口8080启动服务器
}
