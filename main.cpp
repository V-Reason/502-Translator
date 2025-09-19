#include "T:\Coding Header File\ollama\ollama.hpp"]
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

int main() {
	try {
		// 文件操作
		string infileName;
		string outfileName;
		cout << "请输入文件名: ";
		//getline(cin, infileName);
		infileName = "input.txt";
		outfileName = "译文_" + infileName;
		if (!freopen(infileName.c_str(), "r", stdin)
			|| !freopen(outfileName.c_str(), "w", stdout)
			) {
			cerr << "打开文件失败!!!" << endl;
			return 1;
		}

		// 常量配置
		constexpr char kRole[] = "role";
		constexpr char kContent[] = "content";
		constexpr char kThink[] = "thinking";
		constexpr char kUser[] = "user";
		constexpr char kSystem[] = "system";
		constexpr char kAssistant[] = "assistant";
		constexpr char kThinkBeg[] = "<think>";
		constexpr char kThinkEnd[] = "</think>";
		const string quit = "/bye";
		const string model = "deepseek-r1:7b";
		const int maxMess = 30;
		const int num_ctx = 4096; //1024~2048  4096~8192  2048,4096,6144,7680,8192   max:32,768
		// 提示: 可以通过给AI固定格式模版来强化学习, 后期再把模版字符删除
		const string system = "你是ACG汉化组成员，精通英语。请把我发的英语都翻译成通俗生动的中文，不加任何翻译标识符(特别不要擅自添加双引号)和多余的话, 以下是我给你的参考, 请当做参考, 不要给我英语原文,不要解释,不要演绎,不要表演角色, 你的唯一任务就是翻译";
		const bool think = true;
		const bool stream = true;
		//const int keep_alive = 300; //秒

		// 服务开启检测
		if (!ollama::is_running()) {
			cerr << "Ollama服务未启动!";
			return 0;
		}
		else if (!ollama::load_model(model)) {
			cerr << model << "模型无法启动!";
			return 0;
		}

		// 配置参数
		vector<nlohmann::json> messages{
			{{kRole,kSystem},{kContent,system}}
		};
		nlohmann::json options;
		ollama::request request;
		options["num_ctx"] = num_ctx;
		request["model"] = model;
		//request["keep_alive"] = keep_alive;
		request["stream"] = stream;
		request["options"] = options;
		request["messages"] = messages;
		request["think"] = think;

		// 调试
		cerr << "配置文件:" << endl;
		cerr << "Request JSON:\n" << request.dump(4) << endl;

		// 加载学习模版
		ifstream trainfile;
		trainfile.open("trainfile.txt");
		if (!trainfile.is_open()) {
			cerr << "训练文件打开失败!" << endl;
		}
		else {
			while (!trainfile.eof()) {
				string eng, zh;
				getline(trainfile, eng);
				getline(trainfile, zh);
				nlohmann::json temMess = {
					{kRole,kUser},
					{kContent,"原文:" + eng + "\n译文:" + zh}
				};
				request["messages"].emplace_back(temMess);
				//调试
				cerr << "TrainFile JSON:\n" << temMess.dump(2) << endl;
			}
		}
		trainfile.close();
		//调试
		cerr << "Request JSON:\n" << request.dump(4) << endl;

		while (true) {
			//输入
			string input = "请翻译:\n";
			string temp;
			getline(cin, temp);
			input += temp;
			//调试
			static int count = 1; //翻译计数
			cerr << endl;
			cerr << count++ << "、\n";
			cerr << input << endl;

			//退出
			if (input.find(quit) != string::npos
				|| cin.eof()) {
				break;
			}

			//记录用户对话
			nlohmann::json userMessage = {
				{kRole,kUser},
				{kContent,input}
			};
			request["messages"].emplace_back(userMessage);

			//调试
			cerr << endl;
			cerr << "AI: \n";
			
			//流式输出
			string final_res;
			ollama::chat(request, [&](const ollama::response& response) { 

				////调试
				//cerr << "Response JSON:" << endl;
				//cerr << response.as_json().dump(2) << endl;

				//不写入thinking, 只记录正文
				auto json = response.as_json();
				string think = json["message"].value(kThink, "");
				string mess = json["message"].value(kContent, "");

				if (!think.empty()) {
					cerr << think << flush;
				}

				if (!mess.empty()) {
					cerr << mess << flush;
					final_res += mess;
				}

				return true;
				});

			// 写入文件
			cout << count - 1 << ". ";
			cout << final_res << endl << endl;

			//调试
			cerr << endl << endl;
			cerr << "Request JSON:" << endl;
			cerr << request.dump(4);

			// 删除用户对话
			if (!request["messages"].empty()) {
				request["messages"].erase(request["messages"].end() - 1);
			}

			////特化复写
			//final_res = input + "\n译文:\n" + final_res;
			//nlohmann::json templateMess = {
			//	{kRole,kUser},
			//	{kContent,final_res}
			//};
			//request["messages"].emplace_back(templateMess);

			////记录AI对话
			//nlohmann::json AIMessage = {
			//	{kRole,kAssistant},
			//	{kContent,final_res}
			//};
			//request["messages"].emplace_back(AIMessage);

			//限制历史记录数量
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
		//调试
		cerr << endl << endl;
		cerr << "Request JSON:" << endl;
		cerr << request.dump(4);
		cerr << "<<<翻译成功, 请查看本地目录文件!>>>" << endl;
	}
	catch (const exception& e) {
		cerr << "发生错误: " << e.what() << endl;
		return 1;
	}
	return 0;
}

//// 回调函数(已弃用)
//auto response_callback =
//	[](const ollama::response& response) {
//	cout << response << flush;
//	if (response.as_json()["done"] == true)cout << endl;
//	return true;
//	};

// 运行
// (已弃用) bool isThinking = false; //thinking标识符

//会输出thinking(已弃用)
//cout << response.as_simple_string() << flush; 
//final_res += response.as_simple_string();


////旧代码(已弃用)
////调试(查看thinking)
//cerr << chunk << flush;
//if (chunk.find(kThinkBeg) != string::npos) {
//	isThinking = true;
//	return true;//跳过一次, 防止写入<thinking>
//}
//if (chunk.find(kThinkEnd) != string::npos) {
//	isThinking = false;
//	return true;//跳过一次, 防止写入</thinking>
//}
//if (!isThinking) {
//	//cout << chunk << flush;//文件记录操作(注意下面也有一个)
//	final_res += chunk;//对话历史整理操作
//}
//if (response.as_json()["done"] == true) {
//	cerr << endl << endl;
//}

