# d_flash
嵌入式MCU，用于管理flash的一种通用组件。

## 1. 简介
在使用stm32F4系列芯片开发的时候，需要使用其内部flash做一些数据掉电储存，但是芯片只支持扇区擦除，整块擦除。考虑到每次修改数据对整个扇区进行擦除覆写操作是很不靠谱的。  
所以自己开发了这个比较通用的组件。

## 2、原理说明
在扇区的头部，按配置的最大条目数，开辟一个表头空间，存储相关信息。  
每一个条目数据使用，地址+大小+key，进行描述，*其中key是字符串的hash值，hash函数作者（Austin Appleby）*。**对于条目数据的改写是通过链表的方式，为其增加新的数据节点，因此生效的数据是末尾节点的数据。**  
需要重点关注的是如何为其新增数据节点：  
我想到定义一个守护节点链表，链表最末尾的结点始终用于描述未被操作过的最近地址空间。  
*为条目新增节点时，即从最末尾的守护节点处为数据分配一个数据节点*，**之后守护链表继续增加新的末尾节点继续描述之后的未被操作过的最近地址空间。**  

## 3、转储
由于2、我们可以知道生效的始终是链表末尾的节点数据。  
所以转储的核心任务是，将末尾节点的数据转存到目标扇区条目表的最新数据节点。  
所以我们需要定义两块扇区来实现交替存储。

## 4、API


```c
/**
 * @brief 初始化
 * 
 * @param state 剩余空间，或者具体故障码
 * 
 * @return DF_Code 
 * 
 */
DF_Code df_init(void *state);
```

```c
/**
 * @brief 读条目数据
 * 
 * @param key 
 * @param dest 
 * @param size 
 * @return DF_Code 
 */
DF_Code df_read(const char *key, void *dest, uint32_t size);
```

```c
/**
 * @brief 写条目数据
 * 
 * @param key 
 * @param src 
 * @param size 
 * @param state 剩余空间，或者具体故障码
 * @return DF_Code 
 */
DF_Code df_write(const char *key, void *src, uint32_t size, uint32_t *state);
```

```c
/**
 * @brief 强制擦除
 * 
 * @return DF_Code 
 */
DF_Code df_erase(void);

```


## 5、TODO
1. demos为完成
2. 数据长度优化为，任意长度，现在是固定为4的整数倍
