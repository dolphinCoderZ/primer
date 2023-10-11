#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <memory>
#include <sstream>
using namespace std;

class QueryResult;
class TextQuery
{
public:
    using line_no_type = vector<string>::size_type;

    TextQuery(ifstream &is);
    QueryResult query(const string &target) const;

private:
    shared_ptr<vector<string>> file_string;
    // 单词映射到出现的行数集合
    map<string, shared_ptr<set<line_no_type>>> word2map;
};

class QueryResult
{
    friend ostream &printResult(ostream &os, const QueryResult &res);

public:
    using line_no_type = vector<string>::size_type;
    QueryResult(string s, shared_ptr<vector<string>> f, shared_ptr<set<line_no_type>> p) : target(s), file_string(f), row_set(p)
    {
    }

private:
    string target;
    shared_ptr<vector<string>> file_string;
    shared_ptr<set<line_no_type>> row_set;
};

// 读入文件，构建映射
TextQuery::TextQuery(ifstream &is) : file_string(new vector<string>) // string的空vector指针
{
    string text;
    while (getline(is, text))
    {
        this->file_string->push_back(text);
        int row_num = this->file_string->size() - 1;

        istringstream line(text);
        string word;
        while (line >> word)
        {
            auto &row_set = this->word2map[word];
            // 若单词还没有建立映射
            if (!row_set)
            {
                row_set.reset(new set<line_no_type>);
            }
            row_set->insert(row_num);
        }
    }
}

QueryResult TextQuery::query(const string &target) const
{
    static shared_ptr<set<line_no_type>> no_result(new set<line_no_type>);

    auto pos = word2map.find(target);
    if (pos == this->word2map.end())
    {
        return QueryResult(target, this->file_string, no_result);
    }
    else
    {
        return QueryResult(target, this->file_string, pos->second);
    }
}

string make_plural(size_t k, const string &word, const string &ending)
{
    return (k > 1) ? word + ending : word;
}

ostream &printResult(ostream &cout, const QueryResult &res)
{
    cout << res.target << " occurs " << res.row_set->size() << " "
         << make_plural(res.row_set->size(), "time", "s") << endl;

    for (auto num : *res.row_set)
    {
        cout << "\t (line " << num + 1 << ") " << *(res.file_string->begin() + num) << endl;
    }
    return cout;
}

void runQuery(ifstream &infile)
{
    TextQuery tq(infile);
    while (true)
    {
        cout << "enter word to look for, or q to quit: ";
        string s;
        if (!(cin >> s) || s == "q")
        {
            break;
        }
        printResult(cout, tq.query(s)) << endl;
    }
}

int main()
{
    ifstream is("../text_search/text.txt");
    if (is)
    {
        runQuery(is);
    }
    else
    {
        cout << "open failed" << endl;
    }
    return 0;
}
