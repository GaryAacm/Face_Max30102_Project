package config

import (
	"fmt"

	"golang.org/x/crypto/bcrypt"
)

func main() {
	password := "root123"
	hashedPassword := "$2a$10$e6N2as2H5XQ9Kk/NTDWQhOBisYyLkYp9H2KTZkNnRImhP4Bh7Z9em" // 你的哈希密码

	err := bcrypt.CompareHashAndPassword([]byte(hashedPassword), []byte(password))
	if err != nil {
		fmt.Println("Password comparison failed:", err)
	} else {
		fmt.Println("Password comparison succeeded")
	}
}
