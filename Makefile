
OBJS = buddy2.o buddy2_test.o	# 定义一个目标变量，相当于一个宏

target: $(OBJS)
	cc -o target $(OBJS)	# 目标文件target的合成

$(filter %.o,$(OBJS)): %.o: %.c		# 集合：目标集合($@)：依赖集合($<)，filter作过滤
	$(CC) -c $< -o $@	# 依赖比较多时，效率较高

.PHONY: clean			# 特意指明clean是一个伪目标，只能显示执行
clean:
	rm -f target tmp_make $(OBJS) 	# -f表示如果要删除的目标不存在，则忽略
