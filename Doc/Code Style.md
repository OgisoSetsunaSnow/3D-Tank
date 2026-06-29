# Must
- 类名必须以大写开头 InputSystem
- 变量名必须以小写开头 vertexColor
- 常量命名（全局const，全局static，enum，#define，constexpr）必须所有字母的大写并且以下划线分割 COLOR_RED
  - 此外不建议使用#define在代码中而是使用constexpr代替#define
- 函数的名称必须是动词，并且是小写字母开头然后大写字母 changeName()
- 静态函数必须是小写字母s开头并且后续都是大写字母 sGetName()
  - 全局的函数也是如此
- 命名空间的编写则是必须首字母大写然后配合小写
- 英文缩写名词只需要首字母大写就可以了 HTML -> Html openHtml()
- 常见函数命名对照
- ```
    get/set 
    add/remove 
    create/destroy 
    start/stop 
    insert/delete 
    increment/decrement 
    old/new 
    begin/end 
    first/last 
    up/down 
    min/max 
    next/previous 
    open/close 
    show/hide 
    suspend/resume 
- 头文件必须编写 #pragma once include 必须写在文件顶部
- 类中必须用public等标识符标识成员函数和变量是否被暴露，public写在前面
- 类型转换必须进行显示转换（可以使用（）或者static_cast<>）不能接受隐式转换
- 变量的使用只能表示同一个含义，不能一个变量表示两个不同的含义
- for循环的定义中不能添加额外的操作，for()只用于循环控制
for (int sum = 0, i = 0; i < 10; i++) 不能在for循环中额外进行sum变量的初始化

- if 判断中如果布尔变量过多，将多个布尔变量进行分组进行表示
```
bool const isFinished      = (elementNo < 0) || (elementNo > maxElement);
bool const isRepeatedEntry = elementNo == lastElement;
if (isFinished || isRepeatedEntry)
    {
        :
    }
```
- if 判断中不能进行表达式操作
```
if (!(fileHandle = open(filename, "W"))) // 是不好的
fileHandle = open(fileName, "W)
if (fileHandle)
```
# Recommendations
- 智能指针优先使用std::unique_ptr。函数传递参数的时候不要使用智能指针传参。尽量不要使用std::shared_ptr(只有多个地方需要同时拥有这个资源的时候)
- 尽量使用std::array std::vector 来保存数据
- 对于重复逻辑尽量使用for循环进行操作
- 更多建议可以参考Effective C++，Effective Model C++
- 在.h文件中一般使用前向声明，在.cpp中采用完整的include