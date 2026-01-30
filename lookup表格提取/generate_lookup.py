from bs4 import BeautifulSoup
import re

# 寻址模式文本映射到 C 函数名
ADDR_MAP = {
    "immediate":    "&addr_imm",
    "zeropage":     "&addr_zp0",
    "zeropage,X":   "&addr_zpx",
    "zeropage,Y":   "&addr_zpy",
    "absolute":     "&addr_abs",
    "absolute,X":   "&addr_abx",
    "absolute,Y":   "&addr_aby",
    "(indirect,X)": "&addr_izx",
    "(indirect),Y": "&addr_izy",
    "indirect":     "&addr_ind",
    "relative":     "&addr_rel",
    "implied":      "&addr_imp",
    "accumulator":  "&addr_acc",
}

def parse_detailed_tables(filename):
    with open(filename, 'r', encoding='utf-8') as f:
        soup = BeautifulSoup(f, 'html.parser')

    lookup = [None] * 256
    tables = soup.find_all("table")

    for table in tables:
        # === 1. 过滤章节 ===
        # 确保只抓取 standard (details) 和 illegal (illegals) 章节
        # 排除 WDC/Rockwell 等扩展指令集
        section = table.find_previous("h2")
        if not section: continue
        section_id = section.get("id", "")
        if section_id not in ["details", "illegals"]:
            continue

        # === 2. 识别表格类型 ===
        # 检查是否为特殊的 NOPs 表格 (Masswerk 给它们加了 class="nops")
        is_nop_table = "nops" in table.get("class", [])
        
        rows = table.find_all("tr")
        for row in rows:
            cols = row.find_all("td")
            if not cols: continue

            # === 3. 解析不同格式 ===
            
            # 情况 A: 标准表格 (>= 5 列)
            # 格式: addressing | assembler | opc | bytes | cycles
            if len(cols) >= 5:
                addr_text = cols[0].get_text(strip=True)
                asm_text = cols[1].get_text(strip=True)
                opc_text = cols[2].get_text(strip=True)
                cycles_text = cols[4].get_text(strip=True)
                
                # 提取指令名 (例如 "LDA #oper" -> "LDA")
                inst_name = asm_text.split(' ')[0].strip()
                # 统一 JAM 的命名 (网页可能显示为 KIL 或 HLT)
                if inst_name in ["KIL", "HLT"]: inst_name = "JAM"
                
                op_func = f"&op_{inst_name.lower()}"

            # 情况 B: 非官方 NOP 表格 (4 列)
            # 格式: opc | addressing | bytes | cycles
            # 这种表格没有助记符列，但我们知道它们都是 NOP
            elif is_nop_table and len(cols) == 4:
                opc_text = cols[0].get_text(strip=True)
                addr_text = cols[1].get_text(strip=True)
                # bytes 在 cols[2], 这里不需要
                cycles_text = cols[3].get_text(strip=True)
                
                inst_name = "NOP"
                op_func = "&op_nop"
                
            else:
                continue # 跳过表头或其他无关行

            # === 4. 数据转换与存储 ===
            try:
                opcode = int(opc_text, 16)
            except ValueError:
                continue

            mode_func = ADDR_MAP.get(addr_text, "&addr_imp")
            if "accumulator" in addr_text: mode_func = "&addr_acc"

            # 提取周期数 (处理 "2*" 或 "5**" 这种带星号的情况)
            match = re.match(r"(\d+)", cycles_text)
            cycles = int(match.group(1)) if match else 2

            # 存入 lookup 表
            # 注意：如果同一 Opcode 出现多次，此处策略是后出现的覆盖先出现的。
            # 通常标准指令在前，非法在后。如果非法指令是对标准指令的重定义(极少见)，以非法为准。
            # 但在这里主要是填补空缺。
            if lookup[opcode] is None:
                lookup[opcode] = {
                    "name": inst_name,
                    "func": op_func,
                    "mode": mode_func,
                    "cycles": cycles
                }

    return lookup

def generate_c_code(lookup):
    print("// 自动生成的指令表 (包含 Illegal NOPs, 剩余未定义为 JAM)")
    print("Instruction lookup[256] = {")
    
    for i in range(256):
        inst = lookup[i]
        
        if inst:
            # 正常指令 (包含标准指令 + 非法 SLO/RLA 等 + 非法 NOP)
            line = f"    {{ \"{inst['name']}\", {inst['func']}, {inst['mode']}, {inst['cycles']} }},"
            print(f"{line:<55} // 0x{i:02X}")
        else:
            # 真正的死机指令 (JAM/KIL)
            # 网页并没有把 0x02, 0x12 等放入表格，而是作为文本列出。
            # 因此它们会保留为 None，在这里被捕获。
            print(f"    {{ \"JAM\", &op_jam, &addr_imp, 0 }},                 // 0x{i:02X} (JAM/KIL)")
            
    print("};")

if __name__ == "__main__":
    try:
        data = parse_detailed_tables("6502 Instruction Set.html")
        generate_c_code(data)
    except FileNotFoundError:
        print("错误: 找不到文件 '6502 Instruction Set.html'。请先保存网页。")