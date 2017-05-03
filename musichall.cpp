#include<iostream>
#include<sstream>
#include<thread>
#include<fstream>
#include<iterator>
#include<assert.h>
#include<initializer_list>
#include<string>
#include<boost\algorithm\string\classification.hpp>
#include<boost\algorithm\string\split.hpp>
#include<boost\assign\std\map.hpp>
#include<map>
#include<vector>
#include<Windows.h>
#include<time.h>
using namespace std;

//音乐厅共N*M=26*10个座位，6个代理商
#define N 26 
#define M 10
#define K 6

//--------------共享节------------------------------
#pragma data_seg("Shared")
volatile int seat[26][40] = {0};//存放音乐厅座位状态
volatile int flag = 1;//初始化标记
volatile int No = 1; //售票终端编号
volatile int Lock = 0;//0：文件关闭，1:文件打开
#pragma data_seg
#pragma comment(linker,"/Section:Shared,RWS") 

map<string, vector<string>> ticket_record, reserve_record;//存放顾客的订单，包括预定和购买的票
char* str2char(string str);   //string转换char[]
string num2str(int  i);   //数字转string
int alphabet_To_number(char c);//ASCII码转数字
void StringSplit(string s, vector<string>& vec);//string 字符串按空格切分函数，切分结果存储在 vec 中
string char2str(char c);//char转string
string num2str(int  i);//数字转string
int alphabet_To_number(char c);//字母转数字
void tips();//输出提示信息
void getreserve();//获得顾客预定订单
void givereserve();//保存顾客预定订单
void getticket();//获得顾客购买订单
void giveticket();//保存顾客购买订单

 //--------票务中心，集成票务操作 --------------------------
class Ticket_center
{
	map<string, vector<string>>::iterator Iterator_ticket, Iterator_reserve;
public:
	int set_ticket(char in, int im, string customer)//购票
	{
		string temp = char2str(in) + num2str(im);//座位号
		if (seat[alphabet_To_number(in)][im] == 0)
		{
			return 0;
		}
		else if (seat[alphabet_To_number(in)][im] == -1)
		{
			return -1;
		}
		else
		{
			seat[alphabet_To_number(in)][im] = 0;
			getticket();
			Iterator_ticket = ticket_record.find(customer);
			if (Iterator_ticket != ticket_record.end())//在订单中找到了顾客姓名
			{
				ticket_record[customer].push_back(temp);			
			}
			else //没有在订单中找到顾客姓名
			{
				vector<string> ticket_vector;           //ticket vector
				ticket_vector.push_back(temp);
				ticket_record.insert(map<string, vector<string>>::value_type(customer, ticket_vector));
			}
			giveticket();
			return 1;
		}
	}
	int set_cancel(char in, int im, string customer)//取消预定的票
	{
		
		getreserve();
		string temp = char2str(in) + num2str(im);//the ordered seat
		vector<string>::iterator iter = find(reserve_record[customer].begin(), reserve_record[customer].end(), temp);
		if (reserve_record.find(customer)->second.size()==0 || iter== reserve_record[customer].end())
		{
			return 0;
		}
		else
		{
			seat[alphabet_To_number(in)][im] = 1;
			reserve_record[customer].erase(iter);
			givereserve();
			return 1;
		}
	}
	int set_reserve(char in, int im, string customer)//reserve
	{
		string temp = char2str(in) + num2str(im);//the ordered seat
		if (seat[alphabet_To_number(in)][im] == 0)
		{
			return 0;
		}
		else if (seat[alphabet_To_number(in)][im] == -1)
		{
			return -1;
		}
		else
		{
			seat[alphabet_To_number(in)][im] = -1;
			getreserve();
			Iterator_reserve = reserve_record.find(customer);
			if (Iterator_reserve != reserve_record.end())//在订单中找到了顾客姓名
			{
				reserve_record[customer].push_back(temp);
			}
			else //没有在订单中找到顾客姓名
			{
				vector<string> reserve_vector;           //reserve vector
				reserve_vector.push_back(temp);
				reserve_record.insert(map<string, vector<string>>::value_type(customer, reserve_vector));
			}
			givereserve();
			return 1;
		}

	}
	void show_ticket(string customer)
	{
		cout << customer << " 已成功购票： " << endl;
		getticket();
		map<string, vector<string>>::iterator iter = ticket_record.find(customer);
		if (iter != ticket_record.end())
		{
			vector<string>::iterator it = iter->second.begin();
			while (it != iter->second.end())
			{
				cout << (*it++) << "  ";
			}
			cout << endl;
		}
	}
	void show_reserve(string customer)
	{
		cout << customer << " 已成功订票： " << endl;
		getreserve();
		map<string, vector<string>>::iterator iter = reserve_record.find(customer);
		if (iter != reserve_record.end())
		{
			vector<string>::iterator it = iter->second.begin();
			while (it != iter->second.end())
			{
				cout << (*it++) << "  ";
			}
			cout << endl;
		}
	}
	int get_state(int in, int im)
	{
		return seat[in][im];
	}
};
//-------售票代理-----------------------------
class Agent
{
private:
	Ticket_center T;

public:
	Agent(Ticket_center TT)
	{
		T = TT;
	}
	int reserve(char rows, int columns, string customer)
	{
		return T.set_reserve(rows, columns, customer);
	}
	int ticket(char rows, int columns, string customer)
	{
		return T.set_ticket(rows, columns, customer);
	}
	int cancel(char rows, int columns, string customer)
	{
		return T.set_cancel(rows, columns, customer);
	}
	void show_ticket(string customer)
	{
		T.show_ticket(customer);
	}
	void show_reserve(string customer)
	{
		T.show_reserve(customer);
	}
	int get_state(char rows, int columns)
	{
		return T.get_state(rows - 65, columns);
	}
};
void operation(vector<string>::iterator it_start, vector<string>::iterator it_end, Agent* agent,
	vector<string> vStr, int m, int n);//核心函数，票务操作
void agent_operation(Agent* agent);//售票代理操作

//----------------主函数-------------------
int main()
{
	cout << "\n==================您好，欢迎光临，这是第 " << No << " 个音乐厅售票代理！==============\n" << endl;
	if (flag==1)
	{
		for (int i = 0; i < 26; i++)
			for (int j = 0; j < 30; j++)
				seat[i][j] = 1;
		flag++;
		ofstream clear_t("ticket.txt");
		clear_t.close();
		ofstream clear_r("reserve.txt");
		clear_r.close();
	}
	Ticket_center T;
	Agent t(T);
	Agent* agent = &t;
	if (No++>K)
	{
		return 0;
	}
	else
	{
		agent_operation(agent);
	}
	return 0;
}
string num2str(int  i)
{
	stringstream ss;
	ss << i;
	return ss.str();
}
//--------------alphabet to number-------------------
int alphabet_To_number(char c)
{
	return c - 65;
}
//----------------------string to char[]-----------------
char* str2char(string str)
{
	char  c[5];
	strcpy(c, str.c_str());
	return c;
}

//--------------char to string-----------------
string char2str(char c)
{
	string str;
	stringstream stream;
	stream << c;
	str = stream.str();
	return str;
}
void tips()
{
	//-------------------------提示信息------------------------------------------------
	cout << "	请输入您要选择的操作(示例如下)：" << endl;
	cout << "	1、reserve <座位描述> name_of_customer //预定指定座位的票" << endl;
	cout << "	2、ticket <座位描述> name_of_customer  //购买指定座位的票" << endl;
	cout << "	3、cancel <座位描述> name_of_customer  //取消指定座位的预定" << endl;
	cout << "	4、show all name_of_customer           //显示某客户的预定或购票情况" << endl;
	cout << "	PS:客户姓名不包含任何空格且首字母不能是数字\n" << endl;
	cout << "	其中座位描述为：\n	A 2:第A行，第2座\n	A 4 7： 第A行，第4到7座\n	A  ：第A行\n	A 2r  ：从第A行开始的2行" << endl;
	cout << "	A 2c ：第A行，2个座位（不在意具体列）\n	6 ：6个座位（不在意具体行和列）" << endl;
	cout << "===========================================================================" << endl;
}

void operation(vector<string>::iterator it_start, vector<string>::iterator it_end, Agent* agent,
	vector<string> vStr, int n, int m)
{
	if (*it_start == "ticket")
	{
		string customer = *(it_end - 1);
		if (vStr.size() == 3)
		{
			string s = *(it_start + 1);
			char  *c = str2char(s);
			char cr = c[0];
			if (cr <= 'Z'&&cr >= 'A')
			{
				vector<string> fail_seat_temp;
				for (int i = 0; i < m; i++)
				{
					if (agent->ticket(cr, i, customer) != 1)
					{
						string temp = char2str(cr) + num2str(i);
						fail_seat_temp.push_back(temp);
					}
				}
				if (fail_seat_temp.size() != 0)
				{
					cout << "未成功购买座位（被占有或被售出）：" << endl;
					for (vector<string>::iterator it = fail_seat_temp.begin(); it != fail_seat_temp.end(); it++)
					{
						cout << *it << "  ";
					}
					cout << endl;
				}
			}
			else
			{
				int rn = 0, rm = 0;
				for (int i = 0; i < stoi(s); i++)
				{
					while (agent->get_state(rn + 65, rm) != 1)
					{
						rn = rand() % n;
						rm = rand() % m;
					}
					agent->ticket(char(rn + 65), rm, customer);
				}
			}
		}
		else if (vStr.size() == 4)
		{
			string s = *(it_start + 2);
			char  *c1 = str2char(s.c_str());
			char c1r[5], c2r[5];
			strcpy(c1r, c1);
			strcpy(c2r, c1);
			c2r[strlen(c2r) - 1] = '\0';
			if (c1r[strlen(c1r) - 1] == 'r')
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char C_start = ctemp[0];
				string str(c2r);
				vector<string> fail_seat_temp;
				for (int j = 0; j < stoi(str); j++, C_start++)
				{
					
					for (int i = 0; i < m; i++)
					{
						if (agent->ticket(C_start, i, customer) != 1)
						{
							string temp = char2str(C_start) + num2str(i);
							fail_seat_temp.push_back(temp);
						}
					}
					
				}
				if (fail_seat_temp.size() != 0)
				{
					cout << "未成功购买座位（被占有或被售出）：" << endl;
					for (vector<string>::iterator it = fail_seat_temp.begin(); it != fail_seat_temp.end(); it++)
					{
						cout << *it << "  ";
					}
					cout << endl;
				}
			}
			else if (c1[strlen(c1) - 1] == 'c')
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char c_start = ctemp[0];
				string str(c2r);
				int str_num = stoi(str);
				if (str_num > m)
				{
					cout << "音乐厅每行最多 " << m << " 个座位！，请输入小于 " << m << " 的座位数！" << endl;
				}
				else
				{
					int count = 0;
					for (int i = 0; i < m; i++)
					{
						if (agent->get_state(c_start, i) == 1)
						{
							count++;
						}
					}
					if (count < str_num)
					{
						cout << c_start << " 排座位仅剩 " << count << " 个座位， 请选择其它排！";
					}
					else
					{
						int temp = 0;
						for (int i = 0; i < str_num; i++)
						{
							while (agent->get_state(c_start, temp) != 1)
							{
								temp = rand() % m;
							}
							agent->ticket(c_start, temp, customer);
						}
					}
				}
			}
			else
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char c_start = ctemp[0];
				string str(c1r);
				if (agent->ticket(c_start, stoi(str), customer) != 1)
				{
					cout << c_start << str << " 座位不可购买，请选其它座位！" << endl;
				}
			}
		}
		else if (vStr.size() == 5)
		{
			int start = stoi(*(it_start + 2));
			int end = stoi(*(it_start + 3));
			string s = *(it_start + 1);
			char  *c = str2char(s);
			char cc[5];
			strcpy(cc, c);
			int ii = start;
			for (; ii <= end; ii++)
			{
				if (agent->get_state(cc[0], ii) != 1)
					break;
			}
			if (ii > end)
			{
				for (int i = start; i <= end; i++)
				{
					agent->ticket(cc[0], i, *it_end);
				}
			}
			else
			{
				cout << "抱歉，" << c[0] << " 排第 " << start << " 座到 " << end << " 座有座位被占有，请重新选择！" << endl;
			}
		}
		agent->show_ticket(customer);
	}
	else if (*it_start == "reserve")
	{
		string customer = *(it_end - 1);
		if (vStr.size() == 3)
		{
			string s = *(it_start + 1);
			char  *c = str2char(s);
			char cr = c[0];
			if (cr <= 'Z'&& cr >= 'A')
			{
				vector<string> fail_seat_temp;
				for (int i = 0; i < m; i++)
				{
					if (agent->reserve(cr, i, customer) != 1)
					{
						string temp = char2str(cr) + num2str(i);
						fail_seat_temp.push_back(temp);
					}
				}
				if (fail_seat_temp.size() != 0)
				{
					cout << "未成功购买座位（被占有或被售出）：" << endl;
					for (vector<string>::iterator it = fail_seat_temp.begin(); it != fail_seat_temp.end(); it++)
					{
						cout << *it << "  ";
					}
					cout << endl;
				}
			}
			else
			{
				int rn = 0, rm = 0;
				for (int i = 0; i < stoi(s); i++)
				{
					while (agent->get_state(rn + 65, rm) != 1)
					{
						rn = rand() % n;
						rm = rand() % m;
					}
					agent->reserve(char(rn + 65), rm, customer);
				}
			}
		}
		else if (vStr.size() == 4)
		{
			string s = *(it_start + 2);
			char  *c1 = str2char(s.c_str());
			char c1r[5], c2r[5];
			strcpy(c1r, c1);
			strcpy(c2r, c1);
			c2r[strlen(c2r) - 1] = '\0';
			if (c1r[strlen(c1r) - 1] == 'r')
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char C_start = ctemp[0];
				string str(c2r);
				vector<string> fail_seat_temp;
				for (int j = 0; j < stoi(str); j++, C_start++)
				{

					for (int i = 0; i < m; i++)
					{
						if (agent->reserve(C_start, i, customer) != 1)
						{
							string temp = char2str(C_start) + num2str(i);
							fail_seat_temp.push_back(temp);
						}
					}

				}
				if (fail_seat_temp.size() != 0)
				{
					cout << "未成功购买座位（被占有或被售出）：" << endl;
					for (vector<string>::iterator it = fail_seat_temp.begin(); it != fail_seat_temp.end(); it++)
					{
						cout << *it << "  ";
					}
					cout << endl;
				}
			}
			else if (c1[strlen(c1) - 1] == 'c')
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char c_start = ctemp[0];
				string str(c2r);
				int str_num = stoi(str);
				if (str_num > m)
				{
					cout << "音乐厅每行最多 " << m << " 个座位！，请输入小于 " << m << " 的座位数！" << endl;
				}
				else
				{
					int count = 0;
					for (int i = 0; i < m; i++)
					{
						if (agent->get_state(c_start, i) == 1)
						{
							count++;
						}
					}
					if (count < str_num)
					{
						cout << c_start << " 排座位仅剩 " << count << " 个座位， 请选择其它排！";
					}
					else
					{
						int temp = 0;
						for (int i = 0; i < str_num; i++)
						{
							while (agent->get_state(c_start, temp) != 1)
							{
								temp = rand() % m;
							}
							agent->reserve(c_start, temp, customer);
						}
					}
				}
			}
			else
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char c_start = ctemp[0];
				string str(c1r);
				if (agent->reserve(c_start, stoi(str), customer) != 1)
				{
					cout << c_start << str << " 座位不可购买，请选其它座位！" << endl;
				}
			}
		}
		else if (vStr.size() == 5)
		{
			int start = stoi(*(it_start + 2));
			int end = stoi(*(it_start + 3));
			string s = *(it_start + 1);
			char  *c = str2char(s);
			char cc[5];
			strcpy(cc, c);
			int ii = start;
			for (; ii <= end; ii++)
			{
				if (agent->get_state(cc[0], ii) != 1)
					break;
			}
			if (ii > end)
			{
				for (int i = start; i <= end; i++)
				{
					agent->reserve(cc[0], i, *it_end);
				}
			}
			else
			{
				cout << "抱歉，" << c[0] << " 排第 " << start << " 座到 " << end << " 座有座位被占有，请重新选择！" << endl;
			}
		}
		agent->show_reserve(customer);

	}
	else if (*it_start == "cancel")
	{
		string customer = *(it_end - 1);
		vector<string> fail_seat_temp;
		vector<string> success_seat_temp;
		if (vStr.size() == 3)
		{
			string s = *(it_start + 1);
			char  *c = str2char(s);
			char cr = c[0];
			if (cr <= 'Z'&& cr >= 'A')
			{
				for (int i = 0; i < m; i++)
				{
					if (agent->cancel(cr, i, customer) != 0)
					{
						string temp = char2str(cr) + num2str(i);
						success_seat_temp.push_back(temp);
					}
							
				}
			}
			else
			{
				int rn = 0, rm = 0;
				for (int i = 0; i < stoi(s); i++)
				{
					while (agent->get_state(rn + 65, rm) != -1)
					{
						rn = rand() % n;
						rm = rand() % m;
					}
					if (agent->cancel(char(rn + 65), rm, customer) == 1)
					{
						string temp = char2str(cr) + num2str(i);
						success_seat_temp.push_back(temp);
					}
				}
			}
		}
		else if (vStr.size() == 4)
		{
			string s = *(it_start + 2);
			char  *c1 = str2char(s.c_str());
			char c1r[5], c2r[5];
			strcpy(c1r, c1);
			strcpy(c2r, c1);
			c2r[strlen(c2r) - 1] = '\0';
			if (c1r[strlen(c1r) - 1] == 'r')
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char C_start = ctemp[0];
				string str(c2r);
				vector<string> fail_seat_temp;
				for (int j = 0; j < stoi(str); j++, C_start++)
				{

					if (agent->cancel(C_start, j, customer) != -1)
					{
						string temp = char2str(C_start) + num2str(j);
						fail_seat_temp.push_back(temp);
					}
					else
					{
						string temp = char2str(C_start) + num2str(j);
						success_seat_temp.push_back(temp);
					}

				}
			
			}
			else if (c1[strlen(c1) - 1] == 'c')
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char c_start = ctemp[0];
				string str(c2r);
				int str_num = stoi(str);
				if (str_num > m)
				{
					cout << "音乐厅每行最多 " << m << " 个座位！，请输入小于 " << m << " 的座位数！" << endl;
				}
				else
				{
					int count = 0;
					for (int i = 0; i < m; i++)
					{
						if (agent->get_state(c_start, i) == 1)
						{
							count++;
						}
					}
					if (count < str_num)
					{
						cout << c_start << " 排座位仅剩 " << count << " 个座位， 请选择其它排！";
					}
					else
					{
						int temp = 0;
						for (int i = 0; i < str_num; i++)
						{
							while (agent->get_state(c_start, temp) != -1)
							{
								temp = rand() % m;
							}
							if (agent->cancel(c_start, temp, customer) != 1)
							{
								string temp = char2str(c_start) + num2str(i);
								fail_seat_temp.push_back(temp);
							}
							else
							{
								string temp = char2str(c_start) + num2str(i);
								success_seat_temp.push_back(temp);
							}
						}
					}
				}
			}
			else
			{
				string stemp = *(it_start + 1);
				char  *ctemp = str2char(stemp);
				char c_start = ctemp[0];
				string str(c1r);
				if (agent->cancel(c_start, stoi(str), customer) != 1)
				{
					cout << c_start << str << " 座位不可取消，请选择其它座位！" << endl;
				}
				else
				{
					string temp = char2str(c_start) + str;
					success_seat_temp.push_back(temp);
				}
			}
		}
		else if (vStr.size() == 5)
		{
			int start = stoi(*(it_start + 2));
			int end = stoi(*(it_start + 3));
			string s = *(it_start + 1);
			char  *c = str2char(s);
			char cc[5];
			strcpy(cc, c);
			vector<string> fail_seat_temp;
			vector<string> success_seat_temp;
			for (int i = start; i <= end; i++)
			{
				if (agent->cancel(cc[0], i, customer) != -1)
				{
					string temp = char2str(cc[0]) + num2str(i);
					fail_seat_temp.push_back(temp);
				}
				else
				{
					string temp = char2str(cc[0]) + num2str(i);
					success_seat_temp.push_back(temp);
				}
			}
			
		}
		if (fail_seat_temp.size() != 0)
		{
			cout << customer << "未成功取消座位（座位无法取消）：" << endl;
			for (vector<string>::iterator it = fail_seat_temp.begin(); it != fail_seat_temp.end(); it++)
			{
				cout << *it << "  ";
			}
			cout << endl;
		}
		if (success_seat_temp.size() != 0)
		{
			cout << customer << "成功取消座位：" << endl;
			for (vector<string>::iterator it = success_seat_temp.begin(); it != success_seat_temp.end(); it++)
			{
				cout << *it << "  ";
			}
			cout << endl;
		}
	}
	else if (*it_start == "show")
	{
		string customer = *(it_end - 1);
		agent->show_ticket(customer);
		agent->show_reserve(customer);
	}
	else
	{
		cout << "命令输入有误，请重新输入！" << endl;
	}
}
//string 字符串按空格切分函数，切分结果存储在 vec 中
void StringSplit(string s, vector<string>& vec)
{
	if (vec.size() > 0)//保证vec是空的
		vec.clear();
	int length = s.length();
	int start = 0;
	for (int i = 0; i < length; i++)
	{
		if (s[i] == ' ' && i == 0)//第一个就遇到分割符
		{
			start += 1;
		}
		else if (s[i] == ' ')    //遇到空格，将已切分字符串存入vec
		{
			vec.push_back(s.substr(start, i - start));
			start = i + 1;
		}
		else if (i == length - 1)//到达尾部
		{
			vec.push_back(s.substr(start, i + 1 - start));
		}
	}
}
void agent_operation(Agent* agent)
{
	while (true)
	{
		tips();
		string command;
		getline(cin, command);
		vector<string> vStr;
		boost::split(vStr, command, boost::is_any_of(" "), boost::token_compress_on);
		vector<string>::iterator it_start = vStr.begin(), it_end = vStr.end();
		operation(it_start, it_end, agent, vStr, N, M);
	}
}
void getreserve()
{
	while (Lock != 0);//锁机制，等待文件可用
	reserve_record.erase(reserve_record.begin(), reserve_record.end());
	string wordd, inputt;
	vector<string> order;
	ifstream reserve("reserve.txt");
	while (getline(reserve, wordd))
	{
		StringSplit(wordd, order);
		string key = order[0];
		vector<string>::iterator it = order.begin();
		order.erase(it);
		reserve_record.insert(pair<string, vector<string>>(key, order));
	}
	reserve.close();
	Lock = 0;//锁
}

void givereserve()
{
	while (Lock != 0);//锁机制，等待文件可用
	map<string, vector<string>>::iterator it;
	ofstream reserve("reserve.txt");
	for (it = reserve_record.begin(); it != reserve_record.end(); it++)
	{
		reserve << it->first << " ";
		for (vector<string>::iterator itt = (*it).second.begin(); itt != (*it).second.end(); itt++)
			reserve << *itt << " ";
		reserve << "\n";
	}
	reserve.close();
	reserve_record.erase(reserve_record.begin(), reserve_record.end());
	Lock = 0;//锁
}

void getticket()
{
	while (Lock != 0);//锁机制，等待文件可用
	ticket_record.erase(ticket_record.begin(), ticket_record.end());
	string wordd, inputt;
	vector<string> order;
	ifstream ticket("ticket.txt");
	while (getline(ticket, wordd))
	{
		StringSplit(wordd, order);
		string key = order[0];
		vector<string>::iterator it = order.begin();
		order.erase(it);
		ticket_record.insert(pair<string, vector<string>>(key, order));
	}
	ticket.close();
	Lock = 0;//锁
}

void giveticket()
{
	while (Lock != 0);//锁机制，等待文件可用
	map<string, vector<string>>::iterator it;
	ofstream ticket("ticket.txt");
	for (it = ticket_record.begin(); it != ticket_record.end(); it++)
	{
		ticket << it->first << " ";
		for (vector<string>::iterator itt = (*it).second.begin(); itt != (*it).second.end(); itt++)
			ticket << *itt << " ";
		ticket << "\n";
	}
	ticket.close();
	ticket_record.erase(ticket_record.begin(), ticket_record.end());
	Lock = 0;//锁
}