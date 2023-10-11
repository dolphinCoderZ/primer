#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <memory>
#include <sstream>
#include <algorithm>
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
    QueryResult(string s, shared_ptr<vector<string>> f, shared_ptr<set<line_no_type>> p) : target(s), file_string(f), row_set(p) {}

public:
    set<line_no_type>::iterator begin() { return row_set->begin(); }
    set<line_no_type>::iterator end() { return row_set->end(); }
    shared_ptr<vector<string>> get_file() { return this->file_string; }

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
            // 绑定引用
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

class Query_Base
{
    friend class Query;

protected:
    using line_no_type = TextQuery::line_no_type;
    virtual ~Query_Base() = default;

private:
    virtual QueryResult eval(const TextQuery &t) const = 0;
    virtual string rep() const = 0;
};

class Query
{
    // 访问Query的私有构造函数
    friend Query operator~(const Query &expr);
    friend Query operator&(const Query &lhs, const Query &rhs);
    friend Query operator|(const Query &lhs, const Query &rhs);

public:
    // 构建WordQuery
    Query(const string &s);
    QueryResult eval(const TextQuery &t) const { return this->q->eval(t); }
    string rep() const { return this->q->rep(); }

private:
    Query(shared_ptr<Query_Base> query) : q(query) {}
    shared_ptr<Query_Base> q;
};

class WordQuery : public Query_Base
{
    friend class Query;

private:
    WordQuery(const string &s) : query_word(s) {}
    QueryResult eval(const TextQuery &t) const { return t.query(query_word); }
    string rep() const { return query_word; }

private:
    string query_word;
};

inline Query::Query(const string &s) : q(new WordQuery(s)) {}

class NotQuery : public Query_Base
{
    // 访问NotQuery的私有构造函数
    friend Query operator~(const Query &oper);

private:
    NotQuery(const Query &q) : query(q) {}
    string rep() const { return "~(" + query.rep() + ")"; }
    QueryResult eval(const TextQuery &t) const;

private:
    Query query;
};

inline Query operator~(const Query &expr)
{
    shared_ptr<Query_Base> tmp(new NotQuery(expr));
    return Query(tmp);
}

// 二元查询的抽象类
class BinaryQuery : public Query_Base
{
protected:
    BinaryQuery(const Query &l, const Query &r, string s) : lhs(l), rhs(r), symbol(s) {}
    string rep() const { return "(" + lhs.rep() + " " + symbol + " " + rhs.rep() + ")"; }

protected:
    Query lhs, rhs;
    string symbol;
};

class AndQuery : public BinaryQuery
{
    friend Query operator&(const Query &lhs, const Query &rhs);

private:
    AndQuery(const Query &left, const Query &right) : BinaryQuery(left, right, "&") {}
    QueryResult eval(const TextQuery &t) const;
};

inline Query operator&(const Query &lhs, const Query &rhs)
{
    return shared_ptr<Query_Base>(new AndQuery(lhs, rhs));
}

class OrQuery : public BinaryQuery
{
    friend Query operator|(const Query &lhs, const Query &rhs);

private:
    OrQuery(const Query &left, const Query &right) : BinaryQuery(left, right, "|") {}
    QueryResult eval(const TextQuery &t) const;
};

inline Query operator|(const Query &lhs, const Query &rhs)
{
    return shared_ptr<Query_Base>(new OrQuery(lhs, rhs));
}

QueryResult OrQuery::eval(const TextQuery &text) const
{
    auto left = lhs.eval(text);
    auto right = rhs.eval(text);
    auto res = make_shared<set<line_no_type>>(left.begin(), left.end());
    res->insert(right.begin(), right.end());
    return QueryResult(rep(), left.get_file(), res);
}

QueryResult AndQuery::eval(const TextQuery &text) const
{
    auto left = lhs.eval(text);
    auto right = rhs.eval(text);
    auto res = make_shared<set<line_no_type>>();
    set_intersection(left.begin(), left.end(), right.begin(), right.end(), inserter(*res, res->begin()));

    return QueryResult(rep(), left.get_file(), res);
}

QueryResult NotQuery::eval(const TextQuery &text) const
{
    auto result = query.eval(text);
    auto beg = result.begin();
    auto end = result.end();

    auto res = make_shared<set<line_no_type>>();
    auto size = result.get_file()->size();

    for (int i = 0; i != size; i++)
    {
        if (*beg != i || beg == end)
        {
            res->insert(i);
        }
        else if (beg != end)
        {
            ++beg;
        }
    }

    return QueryResult(rep(), result.get_file(), res);
}
