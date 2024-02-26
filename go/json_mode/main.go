package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	"strings"
)

// 定义消息结构体
type Message struct {
	Role    string `json:"role"`
	Content string `json:"content"`
}

// 定义响应结构体
type APIResponse struct {
	Choices []struct {
		Message Message `json:"message"`
	} `json:"choices"`
}

func main() {
	client := &http.Client{}
	reader := bufio.NewReader(os.Stdin)

	// 初始化对话历史，包含符合API要求的系统消息
	var messages = []Message{
		{
			Role:    "system",
			Content: "You are a helpful assistant designed to output in JSON format.",
		},
	}

	for {
		fmt.Print("Enter your question (or 'exit' to quit): ")
		userInput, _ := reader.ReadString('\n')
		userInput = strings.TrimSpace(userInput)
		if userInput == "exit" {
			break
		}

		// 添加用户消息到对话历史
		messages = append(messages, Message{
			Role:    "user",
			Content: userInput,
		})

		// 构造请求体
		requestBody, err := json.Marshal(map[string]interface{}{
			"model":           "gpt-3.5-turbo-0125",
			"messages":        messages,
			"response_format": map[string]string{"type": "json_object"}, // 添加response_format参数
		})
		if err != nil {
			fmt.Println("Error encoding request:", err)
			continue
		}

		// 创建 HTTP 请求
		req, err := http.NewRequest("POST", "https://api.openai.com/v1/chat/completions", bytes.NewBuffer(requestBody))
		if err != nil {
			fmt.Println("Error creating request:", err)
			continue
		}

		// 设置请求头
		req.Header.Add("Content-Type", "application/json")
		req.Header.Add("Authorization", "Bearer "+os.Getenv("OPENAI_API_KEY"))

		// 发送请求
		res, err := client.Do(req)
		if err != nil {
			fmt.Println("Error sending request:", err)
			continue
		}
		defer res.Body.Close()

		// 读取响应
		var response APIResponse
		if err := json.NewDecoder(res.Body).Decode(&response); err != nil {
			fmt.Println("Error decoding response:", err)
			continue
		}

		if len(response.Choices) > 0 {
			fmt.Println("Assistant's response:", response.Choices[0].Message.Content)
			// 将助理的回答添加到对话历史
			messages = append(messages, response.Choices[0].Message)
		} else {
			fmt.Println("No response from assistant.")
		}
	}
}
