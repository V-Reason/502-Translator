#include "T:\Coding Header File\ollama\ollama.hpp"]
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

int main() {
	try {
		// 重定向
		freopen("input.in", "r", stdin);
		// 常量配置
		constexpr char kRole[] = "role";
		constexpr char kContent[] = "content";
		constexpr char kUser[] = "user";
		constexpr char kSystem[] = "system";
		constexpr char kAssistant[] = "assistant";
		constexpr char kThinkBeg[] = "<think>";
		constexpr char kThinkEnd[] = "</think>";
		const string quit = "/bye";
		const string model = "deepseek-r1:7b";
		const int maxMess = 300;
		const int num_ctx = 4096; //1024~2048  4096~8192  2048,4096,6144,7680,8192   max:32,768
		const string system = "你是ACG汉化组成员，精通日语和英语。请把我发的日英文翻译成通俗生动的中文，不改变语言结构，不加任何标识符和多余的话，数字和特殊符号直接输出, 以下是你已经翻译过的中文内容, 供你参考, 不要回复,不要解释,不要演绎,不要表演角色, 你的唯一任务就是翻译";
		const bool stream = true;
		const int keep_alive = 300; //秒

		// 服务开启检测
		if (!ollama::is_running()) {
			cout << "Ollama服务未启动!";
			return 0;
		}
		else if (!ollama::load_model(model)) {
			cout << model << "模型无法启动!";
			return 0;
		}
		// 配置参数
		vector<nlohmann::json> messages{
			{{kRole,kSystem},{kContent,system}}
		//	{{kRole,kUser},{kContent,"This is a testContentFromUser"}},
		//	{{kRole,kAssistant},{kContent,"This is a testContentFromAI"}}
		};
		nlohmann::json options;
		ollama::request request;
		options["num_ctx"] = num_ctx; 
		request["model"] = model;
		request["keep_alive"] = keep_alive;
		request["stream"] = stream;
		request["options"] = options;
		request["messages"] = messages;
		// 调试
		//cout << "配置文件:" << endl;
		//cout << "Request JSON:\n" << request.dump(4) << endl;
		//// 回调函数
		//auto response_callback =
		//	[](const ollama::response& response) {
		//	cout << response << flush;
		//	if (response.as_json()["done"] == true)cout << endl;
		//	return true;
		//	};
		// 运行
		bool isThinking = false; //thinking标识符
		int count = 1; //翻译计数
		while (true) {
			//输入
			string input;
			cout << endl;
			cout << count++ << "、";
			cout << " ";
			getline(cin, input);
			//调试
			cout << input << endl;

			//退出
			if (input == quit)break;
			nlohmann::json message = {
				{kRole,kUser},
				{kContent,input}
			};
			request["messages"].emplace_back(message);//记录用户对话
			string final_res;
			cout << endl;
			cout << "AI: ";
			//流式输出
			ollama::chat(request, [&](const ollama::response& response) { 
				//会输出thinking
				//cout << response.as_simple_string() << flush; 
				//final_res += response.as_simple_string();

				//调试
				//cout << "Response JSON:" << endl;
				//cout << response.as_json().dump(2) << endl;

				//不会输出thinking
				string chunk = response.as_json()["message"][kContent];
				//调试cout << endl << "Chunk:" << chunk << endl;
				cout << chunk << flush;//调试
				if (chunk.find(kThinkBeg) != string::npos) {
					isThinking = true;
					return true;//跳过一次
				}
				if (chunk.find(kThinkEnd) != string::npos) {
					isThinking = false;
					return true;//跳过一次
				}
				if (!isThinking) {//只记录正文
					//cout << chunk << flush;
					final_res += chunk;
				}
				if (response.as_json()["done"] == true)cout << endl;
				return true;
				});
			message = {
				{kRole,kAssistant},
				{kContent,final_res}
			};
			// 删除用户对话
			if (!request["messages"].empty()) {
				request["messages"].erase(request["messages"].end() - 1);
			}
			//记录AI对话
			request["messages"].emplace_back(message);

			// 限制历史记录数量
			if (request["messages"].size() > maxMess) {
				auto beg = request["messages"].begin();
				while (beg != request["messages"].end()//防止越界
					&& (*beg)[kRole] == kSystem) {
					beg++; //跳过system提示词
				}
				if (beg != request["messages"].end()) {  //防止越界
					request["messages"].erase(beg);
				}
			}
		}
		cout << "Request JSON:" << endl;
		cout << request.dump(4);
	}
	catch (const exception& e) {
		cerr << "发生错误: " << e.what() << endl;
		return 1;
	}
	return 0;
}
