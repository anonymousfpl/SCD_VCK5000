output_file = "combined.txt"

with open(output_file, "w") as outfile:
    for i in range(128):
        filename = f"FAMOut_{i}.txt"
        with open(filename, "r") as infile:
            # 只读取前 4096 行
            for j in range(4096):
                line = infile.readline()
                if not line:
                    break  # 如果文件行数不足 4096，则提前退出
                # 每行有两个数字，以空格分隔
                numbers = line.strip().split()
                # 将每个数字单独写入到输出文件，每个数字占一行
                for num in numbers:
                    outfile.write(num + "\n")
