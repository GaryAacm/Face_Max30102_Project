package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/dgrijalva/jwt-go"
	"github.com/gin-gonic/gin"
	"golang.org/x/crypto/bcrypt"
	"gopkg.in/yaml.v3"
	"gorm.io/driver/mysql"
	"gorm.io/gorm"
)

// 定义Auth模型
type Auth struct {
	UserID   string `gorm:"primaryKey"`
	Password string `gorm:"not null"`
	Role     string `gorm:"type:enum('admin','doctor','user');default:'user'"`
}

// TableName 设置Auth模型对应的表名
func (Auth) TableName() string {
	return "auth"
}

// 定义User模型
type User struct {
	UserID         string     `gorm:"primaryKey"`
	Username       string     `gorm:"type:varchar(255)"`
	Gender         string     `gorm:"type:varchar(10)"`
	Birthdate      *time.Time `gorm:"type:date"` // 修改为指针类型
	Age            int
	CreatedAt      time.Time
	FirstVisit     time.Time
	MedicalHistory string `gorm:"type:text"`
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
	UserID string `json:"user_id"`
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
func generateJWT(auth Auth) (string, error) {
	expirationTime := time.Now().Add(24 * time.Hour)
	claims := &Claims{
		UserID: auth.UserID,
		Role:   auth.Role,
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

	// 自动迁移Auth和User模型
	DB.AutoMigrate(&Auth{}, &User{})

	r := gin.Default()
	r.Use(func(c *gin.Context) {
		c.Writer.Header().Set("Access-Control-Allow-Origin", "*")
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
			UserID   string `json:"user_id"`
			Password string `json:"password"`
		}

		if err := ctx.ShouldBindJSON(&req); err != nil {
			ctx.JSON(http.StatusBadRequest, gin.H{"error": "Invalid request format"})
			return
		}

		userID := req.UserID
		password := req.Password

		// 数据验证
		if len(userID) == 0 {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "User ID不能为空",
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

		// 判断UserID是否存在
		var auth Auth
		result := DB.Where("user_id = ?", userID).First(&auth)
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

		// 开始事务
		tx := DB.Begin()

		// 先创建User记录
		newUser := User{
			UserID:     userID,
			CreatedAt:  time.Now(),
			FirstVisit: time.Now(),
		}

		if err := tx.Create(&newUser).Error; err != nil {
			tx.Rollback()
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "用户信息创建失败",
			})
			return
		}

		// 然后创建Auth记录
		newAuth := Auth{
			UserID:   userID,
			Password: string(hashedPassword),
			Role:     "user", // 默认为普通用户
		}

		if err := tx.Create(&newAuth).Error; err != nil {
			tx.Rollback()
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "用户创建失败",
			})
			return
		}

		// 提交事务
		tx.Commit()

		ctx.JSON(http.StatusOK, gin.H{
			"code":    200,
			"message": "注册成功",
		})
	})

	// 登录接口
	r.POST("/login", func(ctx *gin.Context) {
		userID := ctx.PostForm("user_id")
		password := ctx.PostForm("password")

		// 查询用户
		var auth Auth
		result := DB.Where("user_id = ?", userID).First(&auth)
		if result.Error != nil {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "用户不存在",
			})
			return
		}

		// 检查密码
		if err := bcrypt.CompareHashAndPassword([]byte(auth.Password), []byte(password)); err != nil {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "密码错误",
			})
			return
		}

		// 生成JWT
		tokenString, err := generateJWT(auth)
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

	// 获取用户自己的数据（需要登录）
	r.GET("/mydata", authMiddleware(), func(ctx *gin.Context) {
		userID := ctx.GetString("user_id") // 从 JWT 中获取 user_id

		var user User
		if err := DB.Where("user_id = ?", userID).First(&user).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "数据查询失败",
			})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{
			"code": 200,
			"data": user,
		})
	})

	// 管理员登录接口
	r.POST("/admin/login", func(ctx *gin.Context) {
		userID := ctx.PostForm("user_id")
		password := ctx.PostForm("password")

		// 查询管理员
		var admin Auth
		result := DB.Where("user_id = ? AND role = 'admin'", userID).First(&admin)
		if result.Error != nil || bcrypt.CompareHashAndPassword([]byte(admin.Password), []byte(password)) != nil {
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
			UserID   string  `json:"user_id"`
			Password string  `json:"password"`
			Age      *int    `json:"age"`
			Gender   *string `json:"gender"`
		}

		if err := ctx.ShouldBindJSON(&req); err != nil {
			ctx.JSON(http.StatusBadRequest, gin.H{"error": "Invalid request format"})
			return
		}

		if len(req.UserID) == 0 || len(req.Password) < 6 {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "User ID不能为空且密码不能少于6位",
			})
			return
		}

		// 检查用户ID是否已经存在
		var existingUser Auth
		if err := DB.Where("user_id = ?", req.UserID).First(&existingUser).Error; err == nil {
			ctx.JSON(http.StatusConflict, gin.H{"message": "用户ID已存在"})
			return
		}

		// 创建新用户
		hashedPassword, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
		if err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"message": "密码加密错误"})
			return
		}

		newAuth := Auth{
			UserID:   req.UserID,
			Password: string(hashedPassword),
			Role:     "user",
		}

		newUser := User{
			UserID:     req.UserID,
			CreatedAt:  time.Now(), // 设置创建时间为当前时间
			FirstVisit: time.Now(), // 设置首次访问时间为当前时间
			Birthdate:  nil,        // 如果生日留空，将其设置为 NULL
			Age:        0,          // 默认为 0
			Gender:     "",         // 默认为空字符串
		}

		if req.Age != nil {
			newUser.Age = *req.Age
		}
		if req.Gender != nil {
			newUser.Gender = *req.Gender
		}

		if err := DB.Create(&newAuth).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"message": "用户创建失败"})
			return
		}

		if err := DB.Create(&newUser).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"message": "用户信息创建失败"})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{"message": "用户创建成功"})
	})

	// 管理员获取所有用户信息接口（需要管理员权限）
	r.GET("/admin/users", authMiddleware(), func(ctx *gin.Context) {
		role := ctx.GetString("role")
		if role != "admin" {
			ctx.JSON(http.StatusForbidden, gin.H{"code": 403, "message": "无权限"})
			return
		}

		var users []User
		if err := DB.Find(&users).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "用户信息查询失败",
			})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{
			"code": 200,
			"data": users,
		})
	})

	// 管理员更新用户信息接口（需要管理员权限）
	r.PUT("/admin/users/:user_id", authMiddleware(), func(ctx *gin.Context) {
		role := ctx.GetString("role")
		if role != "admin" {
			ctx.JSON(http.StatusForbidden, gin.H{"code": 403, "message": "无权限"})
			return
		}

		var req struct {
			UserID   string  `json:"user_id"`
			Password string  `json:"password"`
			Age      *int    `json:"age"`
			Gender   *string `json:"gender"`
		}

		userID := ctx.Param("user_id")

		if err := ctx.ShouldBindJSON(&req); err != nil {
			ctx.JSON(http.StatusBadRequest, gin.H{"error": "Invalid request format"})
			return
		}

		var auth Auth
		if err := DB.Where("user_id = ?", userID).First(&auth).Error; err != nil {
			ctx.JSON(http.StatusNotFound, gin.H{"code": 404, "message": "用户未找到"})
			return
		}

		if req.Password != "" {
			hashedPassword, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
			if err != nil {
				ctx.JSON(http.StatusInternalServerError, gin.H{"message": "密码加密错误"})
				return
			}
			auth.Password = string(hashedPassword)
		}

		if err := DB.Save(&auth).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "更新失败"})
			return
		}

		var user User
		if err := DB.Where("user_id = ?", userID).First(&user).Error; err != nil {
			ctx.JSON(http.StatusNotFound, gin.H{"code": 404, "message": "用户未找到"})
			return
		}

		if req.Age != nil {
			user.Age = *req.Age
		}
		if req.Gender != nil {
			user.Gender = *req.Gender
		}

		if err := DB.Save(&user).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "更新失败"})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{"code": 200, "message": "更新成功"})
	})

	// 管理员删除用户接口（需要管理员权限）
	r.DELETE("/admin/users/:user_id", authMiddleware(), func(ctx *gin.Context) {
		role := ctx.GetString("role")
		if role != "admin" {
			ctx.JSON(http.StatusForbidden, gin.H{"code": 403, "message": "无权限"})
			return
		}

		userID := ctx.Param("user_id")

		if err := DB.Delete(&Auth{}, "user_id = ?", userID).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "删除失败"})
			return
		}

		if err := DB.Delete(&User{}, "user_id = ?", userID).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "删除失败"})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{"code": 200, "message": "删除成功"})
	})

	r.Run(":8080") // 在端口8080启动服务器
}
