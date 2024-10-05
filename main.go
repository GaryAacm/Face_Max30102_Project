package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"strings"
	"sync"
	"time"

	"github.com/dgrijalva/jwt-go"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"golang.org/x/crypto/bcrypt"
	"gopkg.in/yaml.v3"
	"gorm.io/driver/mysql"
	"gorm.io/gorm"
)

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
	MQTT struct {
		Broker   string `yaml:"broker"`
		ClientID string `yaml:"client_id"`
		Username string `yaml:"username"`
		Password string `yaml:"password"`
		Topic    string `yaml:"topic"`
	} `yaml:"mqtt"`
}

var DB *gorm.DB
var AppConfig *Config

// 全局缓存和同步锁
var sampleCache sync.Map

// CacheEntry 用于缓存 sample_id 和定时器
type CacheEntry struct {
	SampleID string
	Timer    *time.Timer
}

// JWT key used to create the signature
var jwtKey = []byte("123456789")

// Claims struct
type Claims struct {
	UserUUID string `json:"user_uuid"`
	Role     string `json:"role"`
	jwt.StandardClaims
}

func generateUUID() string {
	return uuid.New().String() // 使用 uuid.New() 生成新的 UUID
}

type Auth struct {
	AuthUUID string `gorm:"primaryKey;type:char(36)"`
	UserUUID string `gorm:"type:char(36);not null"` // Foreign Key to User
	Password string `gorm:"not null;type:longtext"`
	Role     string `gorm:"type:enum('admin','doctor','user');default:'user'"`
}

// TableName 设置Auth模型对应的表名
func (Auth) TableName() string {
	return "auth"
}

type User struct {
	UserUUID       string     `gorm:"primaryKey;type:char(36)"`
	UserID         string     `gorm:"unique;type:varchar(255)"`
	Username       string     `gorm:"type:varchar(255)"`
	Gender         string     `gorm:"type:varchar(10)"`
	Birthdate      *time.Time `gorm:"type:date"`
	Age            int
	CreatedAt      time.Time
	FirstVisit     time.Time
	MedicalHistory string `gorm:"type:text"`
}

// TableName 设置User模型对应的表名
func (User) TableName() string {
	return "users"
}

type Sample struct {
	SampleUUID       string    `gorm:"primaryKey;type:char(36)"`
	UserUUID         string    `gorm:"type:char(36)"`
	SampleID         string    `gorm:"unique;type:varchar(255);not null"`
	DeviceID         string    `gorm:"type:varchar(255);not null"`
	Timestamp        time.Time `gorm:"type:timestamp;not null;default:CURRENT_TIMESTAMP"`
	Duration         int       `gorm:"not null;default:0"`
	Operator         string    `gorm:"type:varchar(255)"`
	AssociatedRecord string    `gorm:"type:text"`
	Data             []byte    `gorm:"type:longblob"`
	Category         string    `gorm:"type:enum('type1','type2','type3','type4','type5','unknown');not null"`
}

// TableName 设置 Sample 表名
func (Sample) TableName() string {
	return "samples"
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

func parseDeviceID(sampleID string) (string, error) {
	parts := strings.Split(sampleID, "-")
	if len(parts) < 2 {
		return "", fmt.Errorf("sample_id 格式不正确")
	}
	deviceID := parts[0]
	return deviceID, nil
}

// 验证 sample_id 格式的函数
func isValidSampleID(sampleID string) bool {
	// 根据您的规则实现验证逻辑
	// 这里简单示例：长度大于0
	return len(sampleID) > 0
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
		UserUUID: auth.UserUUID,
		Role:     auth.Role,
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

// 自定义日期格式解析
func parseDate(dateStr string) (*time.Time, error) {
	layout := "2006-01-02"
	parsedDate, err := time.Parse(layout, dateStr)
	if err != nil {
		return nil, err
	}
	return &parsedDate, nil
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

		c.Set("user_uuid", claims.UserUUID)
		c.Set("role", claims.Role)
		c.Next()
	}
}

// 解析 SampleID，获取 DeviceID 和 Timestamp
func parseSampleID(sampleID string) (string, time.Time, error) {
	parts := strings.Split(sampleID, "-")
	if len(parts) < 8 {
		return "", time.Time{}, fmt.Errorf("Sample ID 格式错误")
	}
	deviceID := parts[0]
	timestampStr := fmt.Sprintf("%s-%s-%s %s:%s:%s", parts[1], parts[2], parts[3], parts[4], parts[5], parts[6])
	timestamp, err := time.Parse("2006-01-02 15:04:05", timestampStr)
	if err != nil {
		return "", time.Time{}, fmt.Errorf("时间戳解析错误: %v", err)
	}
	return deviceID, timestamp, nil
}

// 定义分类函数
func getCategoryByChannel(channelID int) (string, error) {
	switch channelID {
	case 0:
		return "type1", nil
	case 1:
		return "type2", nil
	case 3:
		return "type3", nil
	case 5:
		return "type4", nil
	case 7:
		return "type5", nil
	default:
		return "", fmt.Errorf("未知的 channel_id: %d", channelID)
	}
}

// 计算年龄函数
func calculateAge(birthdate time.Time) int {
	today := time.Now()
	age := today.Year() - birthdate.Year()
	if today.YearDay() < birthdate.YearDay() {
		age-- // 如果还没有过今年的生日，年龄减1
	}
	return age
}

// MQTT 客户端
var mqttClient mqtt.Client

func initMQTT() {
	opts := mqtt.NewClientOptions().
		AddBroker(AppConfig.MQTT.Broker).
		SetClientID(AppConfig.MQTT.ClientID).
		SetUsername(AppConfig.MQTT.Username).
		SetPassword(AppConfig.MQTT.Password)
	mqttClient = mqtt.NewClient(opts)
	if token := mqttClient.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	// 订阅主题
	if token := mqttClient.Subscribe(AppConfig.MQTT.Topic, 0, mqttMessageHandler); token.Wait() && token.Error() != nil {
		fmt.Println(token.Error())
	}
}

func mqttMessageHandler(client mqtt.Client, msg mqtt.Message) {
	// 解析 MQTT 消息的 payload
	var data struct {
		SampleID string `json:"sample_id"`
		Data     []byte `json:"data"`
		// 其他字段
	}

	if err := json.Unmarshal(msg.Payload(), &data); err != nil {
		log.Printf("Failed to unmarshal MQTT message: %v", err)
		return
	}

	// 查找对应的 sample 记录
	var sample Sample
	if err := DB.Where("sample_id = ?", data.SampleID).First(&sample).Error; err != nil {
		log.Printf("Sample not found for sample_id: %s", data.SampleID)
		return
	}

	// 更新 sample 的 Data 字段
	sample.Data = data.Data

	// 保存更新后的 sample
	if err := DB.Save(&sample).Error; err != nil {
		log.Printf("Failed to save sample: %v", err)
		return
	}

	log.Printf("Data saved for sample_id: %s", data.SampleID)
}

func main() {
	// 初始化配置、数据库
	InitConfig()

	// 自动迁移Auth、User和Sample模型
	DB.AutoMigrate(&Auth{}, &User{}, &Sample{})

	// 初始化 MQTT
	initMQTT()

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
		var existingUser User
		result := DB.Where("user_id = ?", userID).First(&existingUser)
		if result.RowsAffected > 0 {
			ctx.JSON(http.StatusUnprocessableEntity, gin.H{
				"code":    422,
				"message": "用户已存在",
			})
			return
		}

		// 生成新的UUID
		userUUID := generateUUID()

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
			UserUUID:   userUUID,
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
			AuthUUID: generateUUID(),
			UserUUID: userUUID,
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
		// 查询用户 Auth 信息
		var auth Auth
		result := DB.Where("user_uuid = (SELECT user_uuid FROM users WHERE user_id = ?)", userID).First(&auth)
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

		// 查询用户详细信息
		var user User
		result = DB.Where("user_uuid=?", auth.UserUUID).First(&user)
		if result.Error != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{
				"code":    500,
				"message": "用户信息查询失败",
			})
			return
		}

		// 检查用户信息是否完整
		if user.Username == "" || user.Gender == "" || user.Birthdate == nil || user.Age == 0 {
			ctx.JSON(http.StatusOK, gin.H{
				"code":    200,
				"token":   tokenString,
				"message": "登录成功，但请先完善信息",
				"missing_info": gin.H{
					"username":  user.Username == "",
					"gender":    user.Gender == "",
					"birthdate": user.Birthdate == nil,
					"age":       user.Age == 0,
				},
			})
			return
		}

		// 登录成功，返回 token 和 user_uuid
		ctx.JSON(http.StatusOK, gin.H{
			"code":      200,
			"token":     tokenString,
			"user_uuid": auth.UserUUID, // 返回 user_uuid
			"message":   "登录成功",
		})
	})

	// 更新用户信息接口
	r.POST("/update-info", authMiddleware(), func(ctx *gin.Context) {
		var req struct {
			Username  string `json:"username"`
			Gender    string `json:"gender"`
			Birthdate string `json:"birthdate"`
		}

		// 解析请求体
		if err := ctx.ShouldBindJSON(&req); err != nil {
			log.Printf("JSON binding error: %v", err)
			ctx.JSON(http.StatusBadRequest, gin.H{"error": "Invalid request format"})
			return
		}

		// 将字符串格式的日期转换为时间格式
		birthdate, err := parseDate(req.Birthdate)
		if err != nil {
			ctx.JSON(http.StatusBadRequest, gin.H{"error": "Invalid date format"})
			return
		}

		log.Printf("Received update info: %+v", req) // 记录接收到的更新数据

		// 获取当前用户
		var user User
		if err := DB.Where("user_uuid = ?", ctx.GetString("user_uuid")).First(&user).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "用户信息查询失败"})
			return
		}

		// 更新用户信息
		user.Username = req.Username
		user.Gender = req.Gender
		user.Birthdate = birthdate

		// 自动计算并更新年龄
		user.Age = calculateAge(*birthdate)

		if err := DB.Save(&user).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "用户信息更新失败"})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{"code": 200, "message": "用户信息更新成功"})
	})

	// 获取用户自己的数据（需要登录）
	r.GET("/mydata", authMiddleware(), func(ctx *gin.Context) {
		userUUID := ctx.GetString("user_uuid") // 从 JWT 中获取 user_uuid

		var user User
		if err := DB.Where("user_uuid = ?", userUUID).First(&user).Error; err != nil {
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
	// 客户端绑定接口
	r.POST("/client/bind", authMiddleware(), func(c *gin.Context) {
		var req struct {
			SampleID string `json:"sample_id"`
		}

		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "请求格式错误"})
			return
		}

		sampleID := strings.TrimSpace(req.SampleID)
		if sampleID == "" {
			c.JSON(http.StatusBadRequest, gin.H{"error": "缺少 sample_id"})
			return
		}

		if !isValidSampleID(sampleID) {
			c.JSON(http.StatusBadRequest, gin.H{"error": "无效的 sample_id 格式"})
			return
		}

		userUUID := c.GetString("user_uuid")

		// 解析 sample_id 以提取 device_id
		deviceID, err := parseDeviceID(sampleID)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "无效的 sample_id，无法解析 device_id"})
			return
		}

		// 检查 sample_id 是否已存在
		var existingSample Sample
		result := DB.Where("sample_id = ?", sampleID).First(&existingSample)
		if result.Error == nil {
			// 如果 sample_id 已存在，检查是否已绑定其他用户
			if existingSample.UserUUID != "" && existingSample.UserUUID != userUUID {
				c.JSON(http.StatusConflict, gin.H{"error": "该 sample_id 已被其他用户绑定"})
				return
			} else if existingSample.UserUUID == userUUID {
				c.JSON(http.StatusOK, gin.H{"message": "采样任务已绑定"})
				return
			} else {
				// 绑定当前用户
				existingSample.UserUUID = userUUID
				if err := DB.Save(&existingSample).Error; err != nil {
					c.JSON(http.StatusInternalServerError, gin.H{"error": "绑定采样任务失败"})
					return
				}
				c.JSON(http.StatusOK, gin.H{"message": "采样任务已绑定"})
				return
			}
		} else if result.Error != gorm.ErrRecordNotFound {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "数据库错误"})
			return
		}

		// 创建新的采样记录
		sample := Sample{
			SampleUUID: generateUUID(),
			SampleID:   sampleID,
			UserUUID:   userUUID,
			DeviceID:   deviceID,  // 使用解析后的 device_id
			Category:   "unknown", // 默认设置 category 为 unknown，可以后续更新
			// 其他字段，例如 Timestamp 可以在采集数据时更新
		}

		if err := DB.Create(&sample).Error; err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "创建采样记录失败"})
			return
		}

		c.JSON(http.StatusOK, gin.H{"message": "采样任务已绑定"})
	})

	// 采集端获取 user_uuid 接口
	r.GET("/collect/get_user", func(c *gin.Context) {
		sampleID := strings.TrimSpace(c.Query("sample_id"))
		if sampleID == "" {
			c.JSON(http.StatusBadRequest, gin.H{"error": "缺少 sample_id"})
			return
		}

		var sample Sample
		if err := DB.Where("sample_id = ?", sampleID).First(&sample).Error; err != nil {
			if err == gorm.ErrRecordNotFound {
				c.JSON(http.StatusNotFound, gin.H{"error": "采样记录未找到"})
			} else {
				c.JSON(http.StatusInternalServerError, gin.H{"error": "数据库错误"})
			}
			return
		}

		if sample.UserUUID == "" {
			c.JSON(http.StatusConflict, gin.H{"error": "采样任务未绑定用户"})
			return
		}

		// 直接返回 user_uuid
		c.JSON(http.StatusOK, gin.H{"user_uuid": sample.UserUUID})
	})

	// 采集端提交采集数据接口
	r.POST("/collect/data", func(c *gin.Context) {
		var req struct {
			SampleID string          `json:"sample_id"`
			UserUUID string          `json:"user_uuid"`
			Data     json.RawMessage `json:"data"` // 使用 json.RawMessage 以支持任意 JSON 数据
		}

		// 检查请求体是否符合格式要求
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "请求格式错误"})
			return
		}

		// 去除请求中的空格
		sampleID := strings.TrimSpace(req.SampleID)
		userUUID := strings.TrimSpace(req.UserUUID)

		// 验证请求中是否包含必要的参数
		if sampleID == "" || userUUID == "" || len(req.Data) == 0 {
			c.JSON(http.StatusBadRequest, gin.H{"error": "缺少必要参数"})
			return
		}

		// 查找采样记录
		var sample Sample
		if err := DB.Where("sample_id = ?", sampleID).First(&sample).Error; err != nil {
			if err == gorm.ErrRecordNotFound {
				c.JSON(http.StatusNotFound, gin.H{"error": "采样记录未找到"})
			} else {
				c.JSON(http.StatusInternalServerError, gin.H{"error": "数据库错误"})
			}
			return
		}

		// 检查 sample_id 和 user_uuid 是否匹配
		if sample.UserUUID != userUUID {
			c.JSON(http.StatusForbidden, gin.H{"error": "sample_id 与 user_uuid 不匹配"})
			return
		}

		// 更新采样记录的数据
		sample.Data = req.Data
		sample.Timestamp = time.Now()

		// 保存更新后的采样记录
		if err := DB.Save(&sample).Error; err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "更新采样记录失败"})
			return
		}

		c.JSON(http.StatusOK, gin.H{"message": "采集数据已成功提交"})
	})

	// 管理员登录接口
	r.POST("/admin/login", func(ctx *gin.Context) {
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

		// 查询管理员
		var admin Auth
		result := DB.Where("user_uuid = (SELECT user_uuid FROM users WHERE user_id = ?) AND role = 'admin'", userID).First(&admin)
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
		var existingUser User
		if err := DB.Where("user_id = ?", req.UserID).First(&existingUser).Error; err == nil {
			ctx.JSON(http.StatusConflict, gin.H{"message": "用户ID已存在"})
			return
		}

		// 创建新用户
		userUUID := generateUUID()
		hashedPassword, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
		if err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"message": "密码加密错误"})
			return
		}

		newAuth := Auth{
			AuthUUID: generateUUID(),
			UserUUID: userUUID,
			Password: string(hashedPassword),
			Role:     "user",
		}

		newUser := User{
			UserUUID:   userUUID,
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

		var user User
		if err := DB.Where("user_id = ?", userID).First(&user).Error; err != nil {
			ctx.JSON(http.StatusNotFound, gin.H{"code": 404, "message": "用户未找到"})
			return
		}

		// 更新 Auth 信息
		var auth Auth
		if err := DB.Where("user_uuid = ?", user.UserUUID).First(&auth).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "查询 Auth 信息失败"})
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

		// 更新 User 信息
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

		var user User
		if err := DB.Where("user_id = ?", userID).First(&user).Error; err != nil {
			ctx.JSON(http.StatusNotFound, gin.H{"code": 404, "message": "用户未找到"})
			return
		}

		if err := DB.Delete(&Auth{}, "user_uuid = ?", user.UserUUID).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "删除 Auth 失败"})
			return
		}

		if err := DB.Delete(&User{}, "user_uuid = ?", user.UserUUID).Error; err != nil {
			ctx.JSON(http.StatusInternalServerError, gin.H{"code": 500, "message": "删除 User 失败"})
			return
		}

		ctx.JSON(http.StatusOK, gin.H{"code": 200, "message": "删除成功"})
	})

	r.Run("0.0.0.0:8080") // 在端口8080启动服务器
}
