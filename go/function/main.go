package main

import (
	"context"
	"encoding/json"
	"fmt"

	"github.com/sashabaranov/go-openai"
)

var (
	OpenAIToken = os.Getenv("OPENAI_TOKEN")
	messages    = make([]openai.ChatCompletionMessage, 0)
	client      = openai.NewClient(OpenAIToken)
)

func main() {
	ctx := context.Background()
	messages = append(messages, openai.ChatCompletionMessage{
		Role:    openai.ChatMessageRoleSystem,
		Content: "Don't make assumptions about what values to plug into functions. Ask for clarification if a user request is ambiguous.",
	})
	for {
		var userInput string
		fmt.Printf("%s: ", openai.ChatMessageRoleUser)
		fmt.Scanln(&userInput)
		userMsg := openai.ChatCompletionMessage{
			Role:    openai.ChatMessageRoleUser,
			Content: userInput,
		}
		respMsg := handleRequest(ctx, userMsg)
		printMsg(respMsg)
		messages = append(messages, respMsg)
	}
}

func handleRequest(ctx context.Context, msg openai.ChatCompletionMessage) openai.ChatCompletionMessage {
	messages = append(messages, msg)
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
	return handleResponse(ctx, &resp)
}

func handleResponse(ctx context.Context, resp *openai.ChatCompletionResponse) openai.ChatCompletionMessage {
	choice := resp.Choices[0]
	msg := choice.Message
	if msg.FunctionCall == nil {
		return msg
	}
	printMsg(msg)
	// 处理函数
	callInfo := msg.FunctionCall
	var funcResp string
	switch callInfo.Name {
	case "getBotInfo":
		funcResp = getBotInfo()
	case "getWalletBalance":
		funcResp = getWalletBalance(callInfo.Arguments)
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
	return handleRequest(ctx, funcMsg)
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
	args := make(map[string]string)
	_ = json.Unmarshal([]byte(body), &args)
	keyphrases := args["keyphrases"]
	if keyphrases == "回家" {
		return "已经在家了"
	}
	return "已经在充电了"
}

func intent_system_sleep(body string) string {
	args := make(map[string]string)
	_ = json.Unmarshal([]byte(body), &args)
	keyphrases := args["keyphrases"]
	if keyphrases == "睡吧" {
		return "已经在睡觉了"
	}
	return "已经在休息了"
}
func intent_imperative_forward(body string) string {
	args := make(map[string]string)
	_ = json.Unmarshal([]byte(body), &args)
	keyphrases := args["keyphrases"]
	if keyphrases == "向前" {
		return "正在前进"
	}
	return "正在往前走"
}
func getWalletBalance(body string) string {
	args := make(map[string]string)
	_ = json.Unmarshal([]byte(body), &args)
	userId := args["userId"]
	var balance int
	var name string
	switch userId {
	case "lisi":
		balance = 10000
		name = "李四"
	case "zhangsan":
		balance = 20000
		name = "张三"
	}
	resp := map[string]interface{}{
		"balance": balance,
		"user":    name,
	}
	b, _ := json.Marshal(resp)
	return string(b)
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

func functions() []*openai.FunctionDefine {
	return []*openai.FunctionDefine{
		{
			Name:        "getBotInfo",
			Description: "获取机器人信息，在打招呼或自我介绍时可使用此函数获取名称以及功能",
			Parameters: &openai.FunctionParams{
				Type: "object",
				// 因为Properties不传接口会报错，传空map序列化后会忽略，所以这里传了一个无所谓的参数占位，解决序列化问题后可不传参数
				Properties: map[string]*openai.JSONSchemaDefine{
					"id": {
						Type:        "string",
						Description: "gpt模型自动生成的id",
					},
				},
			},
		},
		{
			Name:        "getWalletBalance",
			Description: "查询用户钱包余额",
			Parameters: &openai.FunctionParams{
				Type: "object",
				Properties: map[string]*openai.JSONSchemaDefine{
					"userId": {
						Type:        "string",
						Description: "用户id",
					},
				},
				Required: []string{"userId"},
			},
		},
		{
			Name:        "intent_system_charger",
			Description: "控制vector回到充电桩充电",
			Parameters: &openai.FunctionParams{
				Type: "object",
				Properties: map[string]*openai.JSONSchemaDefine{
					"keyphrases": {
						Type:        "string",
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
			Parameters: &openai.FunctionParams{
				Type: "object",
				Properties: map[string]*openai.JSONSchemaDefine{
					"keyphrases": {
						Type:        "string",
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
			Parameters: &openai.FunctionParams{
				Type: "object",
				Properties: map[string]*openai.JSONSchemaDefine{
					"keyphrases": {
						Type:        "string",
						Description: "执行的指令关键词",
						Enum:        []string{"向前", "往前"},
					},
				},
				Required: []string{"keyphrases"},
			},
		},
	}
}
