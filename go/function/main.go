package main

import (
	"context"
	"encoding/json"
	"fmt"
	"os"
	"sync"

	"github.com/sashabaranov/go-openai"
	"github.com/sashabaranov/go-openai/jsonschema"
)

var (
	OpenAIToken = os.Getenv("OPENAI_TOKEN")
	client      = openai.NewClient(OpenAIToken)
)

func main() {
	ctx := context.Background()
	messages := make([]openai.ChatCompletionMessage, 0)
	messages = append(messages, openai.ChatCompletionMessage{
		Role:    openai.ChatMessageRoleSystem,
		Content: "Don't make assumptions about what values to plug into functions. Ask for clarification if a user request is ambiguous.",
	})

	inputChan := make(chan string)
	responseChan := make(chan openai.ChatCompletionMessage)
	doneChan := make(chan bool) // 用于同步处理完成的信号
	var wg sync.WaitGroup

	wg.Add(1)
	go func() {
		defer wg.Done()
		for {
			var userInput string
			fmt.Printf("User: ")
			fmt.Scanln(&userInput)
			if userInput == "exit" {
				close(inputChan) // 关闭channel以通知主线程退出
				return
			}
			inputChan <- userInput
			<-doneChan // 等待处理完成的信号
		}
	}()

	// 处理请求goroutine
	wg.Add(1)
	go func(ctx context.Context, inputChan <-chan string, responseChan chan<- openai.ChatCompletionMessage) {
		defer wg.Done()
		messages := make([]openai.ChatCompletionMessage, 0)
		for userInput := range inputChan {
			userMsg := openai.ChatCompletionMessage{
				Role:    openai.ChatMessageRoleUser,
				Content: userInput,
			}
			respMsg := handleRequest(ctx, userMsg, messages)
			messages = append(messages, userMsg, respMsg)
			responseChan <- respMsg
		}
		close(responseChan)
	}(ctx, inputChan, responseChan)

	// 打印响应
	go func(responseChan <-chan openai.ChatCompletionMessage) {
		for respMsg := range responseChan {
			printMsg(respMsg)
			doneChan <- true // 发送处理完成的信号
		}
	}(responseChan)

	wg.Wait()
}

func handleRequest(ctx context.Context, initialMsg openai.ChatCompletionMessage, messages []openai.ChatCompletionMessage) openai.ChatCompletionMessage {
	var lastMsg openai.ChatCompletionMessage = initialMsg

	for {
		messages = append(messages, lastMsg)
		funcs := functions()
		req := openai.ChatCompletionRequest{
			Model:     openai.GPT3Dot5Turbo0613,
			Functions: funcs,
			Messages:  messages,
		}
		resp, err := client.CreateChatCompletion(ctx, req)
		if err != nil {
			panic(err)
		}
		// 处理响应，如果没有函数调用则跳出循环
		choice := resp.Choices[0]
		msg := choice.Message
		if msg.FunctionCall == nil {
			return msg
		} else {
			lastMsg = handleResponse(ctx, &resp)
		}
	}
}

func handleResponse(ctx context.Context, resp *openai.ChatCompletionResponse) openai.ChatCompletionMessage {
	choice := resp.Choices[0]
	msg := choice.Message
	printMsg(msg)
	// 处理函数调用逻辑
	callInfo := msg.FunctionCall
	var funcResp string
	switch callInfo.Name {
	case "getBotInfo":
		funcResp = getBotInfo()
	case "intent_system_charger":
		funcResp = intent_system_charger(callInfo.Arguments)
	case "intent_system_sleep":
		funcResp = intent_system_sleep(callInfo.Arguments)
	case "intent_imperative_forward":
		funcResp = intent_imperative_forward(callInfo.Arguments)
	}
	funcMsg := openai.ChatCompletionMessage{
		Role:    openai.ChatMessageRoleFunction,
		Content: funcResp,
		Name:    callInfo.Name,
	}
	printMsg(funcMsg)
	return funcMsg
}

func printMsg(msg openai.ChatCompletionMessage) {
	role := msg.Role
	content := msg.Content
	if content == "" {
		contentBytes, _ := json.Marshal(msg)
		content = string(contentBytes)
	}
	fmt.Printf("%s: %s\n", role, content)
}

func functions() []openai.FunctionDefinition {
	return []openai.FunctionDefinition{
		{
			Name:        "getBotInfo",
			Description: "获取机器人信息，在打招呼或自我介绍时可使用此函数获取名称以及功能",
			Parameters: &jsonschema.Definition{
				Type: "object",
				Properties: map[string]jsonschema.Definition{
					"id": {
						Type:        jsonschema.String,
						Description: "gpt模型自动生成的id",
					},
				},
			},
		},
		{
			Name:        "intent_system_charger",
			Description: "控制vector回到充电桩充电",
			Parameters: &jsonschema.Definition{
				Type: "object",
				Properties: map[string]jsonschema.Definition{
					"keyphrases": {
						Type:        jsonschema.String,
						Description: "执行的指令关键词",
						Enum:        []string{"回家", "充电"},
					},
				},
				Required: []string{"keyphrases"},
			},
		},
		{
			Name:        "intent_system_sleep",
			Description: "控制vector进入休眠状态",
			Parameters: &jsonschema.Definition{
				Type: "object",
				Properties: map[string]jsonschema.Definition{
					"keyphrases": {
						Type:        jsonschema.String,
						Description: "执行的指令关键词",
						Enum:        []string{"睡吧", "睡觉"},
					},
				},
				Required: []string{"keyphrases"},
			},
		},
		{
			Name:        "intent_imperative_forward",
			Description: "控制vector前进",
			Parameters: &jsonschema.Definition{
				Type: "object",
				Properties: map[string]jsonschema.Definition{
					"keyphrases": {
						Type:        jsonschema.String,
						Description: "执行的指令关键词",
						Enum:        []string{"向前", "往前"},
					},
				},
				Required: []string{"keyphrases"},
			},
		},
	}
}

func getBotInfo() string {
	body := map[string]string{
		"name":              "胡图图",
		"features":          "胡图图是一个具有人工智能的虚拟角色，其主要功能是根据用户的输入提供有趣的对话和执行简单的控制指令",
		"self-introduction": "我叫胡图图，今年三岁，我的爸爸叫胡英俊，我的妈妈叫张小丽，我家住在翻斗花园二号楼一零零一室，妈妈做的炸小肉丸最好吃，我的猫咪叫小怪，他是一只会说话的猫咪呦，小怪和图图一样是个男孩子，图图最喜欢的好朋友是小美，图图的耳朵很大很神奇，你们看动耳神功，请问有没有烤肉串呢，那炸臭豆腐呢，那随便来一个烤红薯好了，有木有冰淇淋巧克力彩虹糖，旺旺饼干花生米，牛肉干豆奶酸奶橘子汁胡萝卜汁苹果汁",
		"author":            "wangergou",
		"version":           "1.0",
	}
	b, _ := json.Marshal(body)
	return string(b)
}

func intent_system_charger(body string) string {
	fmt.Printf("参数: %s\n", body)
	return "正在去往充电桩"
}

func intent_system_sleep(body string) string {
	fmt.Printf("参数: %s\n", body)
	return "已经在睡觉了"
}
func intent_imperative_forward(body string) string {
	fmt.Printf("参数: %s\n", body)
	return "正在往前走"
}
