#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

// 伙伴分配器的数据结构
struct buddy2 {
  unsigned size;		// 管理内存的总单元数目，unsigned后省略的是int
  unsigned longest[1];	// 二叉树的节点标记，表明所对应的内存块的空闲单元 
  						// longest记录了节点所对应的内存块大小？？？
};

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))	// 利用2的幂次方只有1个bit为1判断x是否为2的幂次方

#define ALLOC malloc
#define FREE free

// 分配器初始化
void main()
{
  	struct buddy2* self;
  	unsigned node_size;
  	int i,size=32;

  	self = (struct buddy2*)ALLOC( 2 * size * sizeof(unsigned));
  	self->size = size;		// 第一个结构体的size存放总的块数size
  	node_size = size * 2;		// 总共的sizeof(unsigned)数

  	for (i = 0; i < 2 * size - 1; ++i) {
  	  if (IS_POWER_OF_2(i+1))
  	    node_size /= 2;
  	  self->longest[i] = node_size;		// 经验证，这里可以正确向后访问 
  	}

  	printf("memories allocated:\r\n");
	unsigned* init = (unsigned*)self;
  	for(i=0; i< 2 * size; i++,init++)
  	{
  		printf("%d\n",*init);
  	}

	// 一下是验证longest访问正确性的
	//unsigned* init = (unsigned*)self;
  	//for(i=0; i<64; i++,init++)
  	//{
  	//    *init = i;
  	//}

  	//init = (unsigned*)self;
  	//printf("memories allocated:\r\n");
  	//for(i=0; i<64; i++,init++)
  	//	printf("%d\t",*init);
  	//for(i=0; i<32; i++)
  	//	printf("self->size=%d\tself->longest[%d]=%d\r\n",self->size,i,self->longest[i]);
}

