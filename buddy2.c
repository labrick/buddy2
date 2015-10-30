#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// 伙伴分配器的数据结构
struct buddy2 {
  unsigned size;		// 管理内存的总单元数目，unsigned后省略的是int
  unsigned longest[1];	// 二叉树的节点标记，表明该节点可分配的最大单元数
};

#define LEFT_LEAF(index) ((index) * 2 + 1)		// 计算index的左子树索引号
#define RIGHT_LEAF(index) ((index) * 2 + 2)		// 类似上
#define PARENT(index) ( ((index) + 1) / 2 - 1)	// 找到index的父节点索引号

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))	// 利用2的幂次方只有1个bit为1判断x是否为2的幂次方
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ALLOC malloc
#define FREE free

/*
 * 把size调整到向上取到最近的2次幂
 * 方法：利用2次幂的二进制中有且仅有一个比特位为1，将从高位到低位数出现的
 * 第一个1依次复制到后面的所有位中，然后+1即得到仅大于size的2次幂
 */
static unsigned fixsize(unsigned size) {
  size |= size >> 1;	// 将第一个1复制到连续的后一位(2个1)
  size |= size >> 2;	// 将前两个1复制到连续的后两位(4个1)
  size |= size >> 4;	// 将前四个1复制到连续的后四位(8个1)
  size |= size >> 8;	// 将前八个1复制到连续的后八位(16个1)
  size |= size >> 16;	// 将前十六个1复制到连续的后十六位(32个1)
  return size+1;
}

// 分配器初始化
struct buddy2* buddy2_new( int size ) {
  struct buddy2* self;
  unsigned node_size;		// 存放每个节点空闲空间的临时变量
  int i;

  if (size < 1 || !IS_POWER_OF_2(size))	// 所管理的内存单元数必须合格
    return NULL;

// 申请size个结构体的空间用来存储所需要的结构
  self = (struct buddy2*)ALLOC( 2 * size * sizeof(unsigned));
  self->size = size;		// 第一个结构体的size存放总的块数size
  node_size = size * 2;		// 为/2做准备

  for (i = 0; i < 2 * size - 1; ++i) {
    if (IS_POWER_OF_2(i+1))
      node_size /= 2;
    self->longest[i] = node_size;		// 经验证，这里可以正确向后访问 
  }
  return self;
}

// 作为伙伴系统测试的内存销毁
void buddy2_destroy( struct buddy2* self) {
  FREE(self);
}

int buddy2_alloc(struct buddy2* self, int size) {
  unsigned index = 0;
  unsigned node_size;
  unsigned offset = 0;

  if (self==NULL)
    return -1;

  if (size <= 0)
    size = 1;
  else if (!IS_POWER_OF_2(size))
    size = fixsize(size);

  if (self->longest[index] < size)
    return -1;

  // 每次将self->size半切割，直到切为所要分配的大小size为止，保留此时的索引值
  for(node_size = self->size; node_size != size; node_size /= 2 ) {
    if (self->longest[LEFT_LEAF(index)] >= size)
      index = LEFT_LEAF(index);
    else
      index = RIGHT_LEAF(index);
  }

  self->longest[index] = 0;		// 找到要分配的空间，将其标志置0
  offset = (index + 1) * node_size - self->size;
  // offset = (index - 第一个节点) * node_size
  // 		= (index + 1 - 第二个节点) * node_size
  // 		= (index + 1) * node_size - 第二个节点 * node_size
  // 		= (index + 1) * node_size - self->size
  // index与offset的关系如下:
  // ----------------------------------------------
  // 0---------------------------------------------
  // 1----------------------2----------------------
  // 3-----------4----------5-----------6----------
  // 7-----8-----9----10----11----12----13---14----

  while (index) {		// 更新所有父节点处最大可分配的数值 
    index = PARENT(index);
    self->longest[index] = 
      MAX(self->longest[LEFT_LEAF(index)], self->longest[RIGHT_LEAF(index)]);
  }

  return offset;
}

void buddy2_free(struct buddy2* self, int offset) {
  unsigned node_size, index = 0;
  unsigned left_longest, right_longest;

  assert(self && offset >= 0 && offset < self->size);

  node_size = 1;	// 从子树开始搜索处第一个被标记的节点
  index = offset + self->size - 1;

  for (; self->longest[index] ; index = PARENT(index)) {
    node_size *= 2;
    if (index == 0)
      return;	// 说明是根节点，不需要更新父节点
  }

  self->longest[index] = node_size;		// 还原为之前的可分配的最大块数

  while (index) {		// 以此更新父节点
    index = PARENT(index);
    node_size *= 2;

    left_longest = self->longest[LEFT_LEAF(index)];
    right_longest = self->longest[RIGHT_LEAF(index)];
    
    if (left_longest + right_longest == node_size) 
      self->longest[index] = node_size;
    else
      self->longest[index] = MAX(left_longest, right_longest);
  }
}

// 根据offset获得该块内存的大小，同free
int buddy2_size(struct buddy2* self, int offset) {
  unsigned node_size, index = 0;

  assert(self && offset >= 0 && offset < self->size);

  node_size = 1;	// 从子节点开始网上搜索直到出现longest为0，则此时的node_size即为块大小
  for (index = offset + self->size - 1; self->longest[index] ; index = PARENT(index))
    node_size *= 2;

  return node_size;
}

// 采用canvas数组模拟需要操作的内存，并用*标识已分配部分，_表示未分配部分
void buddy2_dump(struct buddy2* self) {
  char canvas[65];		// 模拟需要操作的内存块
  int i,j;
  unsigned node_size, offset;

  if (self == NULL) {
    printf("buddy2_dump: (struct buddy2*)self == NULL");
    return;
  }

  if (self->size > 64) {
    printf("buddy2_dump: (struct buddy2*)self is too big to dump");
    return;
  }

  memset(canvas,'_', sizeof(canvas));
  node_size = self->size * 2;		// 节点0对应的内存块大小

  for (i = 0; i < 2 * self->size - 1; ++i) {	// 对所有节点进行遍历
    if ( IS_POWER_OF_2(i+1) )
      node_size /= 2;

    if ( self->longest[i] == 0 ) {		// 如果节点已被分配
      if (i >=  self->size - 1) {		// 属于后一半索引值，也就是所分配的只有一个单元
        canvas[i - self->size + 1] = '*';	// 给对应的canvas标记*
      }
      else if (self->longest[LEFT_LEAF(i)] && self->longest[RIGHT_LEAF(i)]) {	// 非后半索引值，且左右子树有为0时
        offset = (i+1) * node_size - self->size;		// 找到它的内存偏移值

        for (j = offset; j < offset + node_size; ++j)	// 将找到的块内存对应的canvas标记为*
          canvas[j] = '*';
      }
    }
  }
  canvas[self->size] = '\0';
  puts(canvas);
}
