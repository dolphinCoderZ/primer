#include <iostream>
#include <memory>
using namespace std;

class StrVec
{
public:
    StrVec() : head(nullptr), first_free(nullptr), cap(nullptr) {}
    // 普通拷贝
    StrVec(const StrVec &rhs);
    StrVec &operator=(const StrVec &rhs);

    // 移动拷贝
    StrVec(StrVec &&rhs) noexcept;
    StrVec &operator=(StrVec &&rhs) noexcept;

    // 初始化器赋值
    StrVec &operator=(initializer_list<string> il);
    ~StrVec();

    string &operator[](int n) { return this->head[n]; }
    const string &operator[](int n) const { return this->head[n]; }

    void push_back(const string &s);
    int size() const { return this->first_free - this->head; }
    int capacity() const { return this->cap - this->head; }

    string *begin() const { return this->head; }
    string *end() const { return this->first_free; }

private:
    // 分配内存
    static allocator<string> alloc;

    void reallocate();
    void chk_n_alloc()
    {
        if (this->size() == this->capacity())
        {
            this->reallocate();
        }
    }
    void free();
    pair<string *, string *> alloc_n_copy(const string *begin, const string *end);

    string *head;
    string *first_free;
    string *cap;
};

void StrVec::push_back(const string &s)
{
    this->chk_n_alloc();
    this->alloc.construct(this->first_free++, s);
}

pair<string *, string *> StrVec::alloc_n_copy(const string *begin, const string *end)
{
    auto data = this->alloc.allocate(end - begin);
    return {data, uninitialized_copy(begin, end, data)};
}

void StrVec::free()
{
    if (this->head)
    {
        for (auto p = this->first_free; p != this->head;)
        {
            alloc.destroy(--p);
        }
        alloc.deallocate(this->head, this->capacity());
    }
}

StrVec::StrVec(const StrVec &rhs)
{
    auto new_data = this->alloc_n_copy(rhs.begin(), rhs.end());
    this->head = new_data.first;
    this->first_free = new_data.second;
    this->cap = new_data.second;
}

StrVec::StrVec(StrVec &&rhs) noexcept : head(rhs.head), first_free(rhs.first_free), cap(rhs.cap)
{
    rhs.head = nullptr;
    rhs.first_free = nullptr;
    rhs.cap = nullptr;
}

StrVec &StrVec::operator=(const StrVec &rhs)
{
    auto new_data = this->alloc_n_copy(rhs.begin(), rhs.end());
    this->free();

    this->head = new_data.first;
    this->first_free = new_data.second;
    this->cap = new_data.second;
    return *this;
}

StrVec &StrVec::operator=(StrVec &&rhs) noexcept
{
    if (this != &rhs)
    {
        this->free();
        this->head = rhs.head;
        this->first_free = rhs.first_free;
        this->cap = rhs.cap;

        rhs.head = nullptr;
        rhs.first_free = nullptr;
        rhs.cap = nullptr;
    }
    return *this;
}

StrVec::~StrVec()
{
    this->free();
}

StrVec &StrVec::operator=(initializer_list<string> il)
{
    auto data = this->alloc_n_copy(il.begin(), il.end());

    this->free();
    this->head = data.first;
    this->first_free = data.second;
    this->cap = data.second;
    return *this;
}

void StrVec::reallocate()
{
    auto new_capacity = this->size() ? 2 * this->size() : 1;
    auto new_data = this->alloc.allocate(new_capacity);

    auto dest = new_data;
    auto origial = this->head;

    for (int i = 0; i < this->size(); i++)
    {
        this->alloc.construct(dest++, std::move(*origial++));
    }
    this->free();

    this->head = new_data;
    this->first_free = dest;
    this->cap = this->head + new_capacity;
}
