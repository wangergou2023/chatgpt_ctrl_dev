package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
)

type APIResponse struct {
	Choices []struct {
		Message struct {
			Role    string `json:"role"`
			Content string `json:"content"`
		} `json:"message"`
	} `json:"choices"`
}

func main() {
	client := &http.Client{}
	reader := bufio.NewReader(os.Stdin)

	for {
		fmt.Print("Enter your question (or 'exit' to quit): ")
		userInput, _ := reader.ReadString('\n')
		userInput = strings.TrimSpace(userInput)
		if userInput == "exit" {
			break
		}

		requestBody, err := json.Marshal(map[string]interface{}{
			"model": "gpt-3.5-turbo",
			"messages": []map[string]string{
				{
					"role":    "user",
					"content": userInput,
				},
			},
		})
		if err != nil {
			fmt.Println("Error encoding request:", err)
			continue
		}

		req, err := http.NewRequest("POST", "https://api.openai.com/v1/chat/completions", bytes.NewBuffer(requestBody))
		if err != nil {
			fmt.Println("Error creating request:", err)
			continue
		}

		req.Header.Add("Content-Type", "application/json")
		req.Header.Add("Authorization", "Bearer "+os.Getenv("OPENAI_API_KEY"))

		res, err := client.Do(req)
		if err != nil {
			fmt.Println("Error sending request:", err)
			continue
		}
		defer res.Body.Close()

		body, err := io.ReadAll(res.Body)
		if err != nil {
			fmt.Println("Error reading response:", err)
			continue
		}

		var response APIResponse
		if err := json.Unmarshal(body, &response); err != nil {
			fmt.Println("Error parsing response:", err)
			continue
		}

		if len(response.Choices) > 0 && response.Choices[0].Message.Role == "assistant" {
			fmt.Println("Assistant's response:", response.Choices[0].Message.Content)
		} else {
			fmt.Println("No response from assistant.")
		}
	}
}
