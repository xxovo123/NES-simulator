# **iNES 文件格式技术规格书**

.nes 文件格式（iNES 格式）是 NES（FC）二进制程序的行业标准。它最初由 Marat Fayzullin 为其编写的 iNES 模拟器开发，后来成为所有 NES 模拟器的通用标准，甚至被用于商业授权产品（如 PocketNES 和 Wii Virtual Console）。

## **1\. 文件结构 (File Structure)**

一个完整的 iNES 文件按顺序包含以下部分：

1. **文件头 (Header)**：16 字节。  
2. **Trainer (训练段)**：可选，0 或 512 字节（位于 PRG 数据之前）。  
3. **PRG ROM 数据**：![][image1] 字节（16 KB 的倍数）。  
4. **CHR ROM 数据**：可选，![][image2] 字节（8 KB 的倍数）。如果为 0，则该板卡使用 CHR RAM。  
5. **PlayChoice INST-ROM**：可选，0 或 8192 字节。  
6. **PlayChoice PROM**：可选，16 字节数据 \+ 16 字节 CounterOut。

某些文件末尾可能还包含 127 或 128 字节的标题信息（非标准）。

## **2\. 文件头格式 (Header Detail)**

文件头共 16 字节，具体定义如下：

| 字节 | 描述 |
| :---- | :---- |
| **0-3** | 魔数：$4E $45 $53 $1A (ASCII 字符 "NES" 后跟 MS-DOS 换行符) |
| **4** | PRG ROM 大小（以 16 KB 为单位） |
| **5** | CHR ROM 大小（以 8 KB 为单位）。值为 0 表示使用 CHR RAM |
| **6** | **Flags 6**：Mapper 低 4 位、镜像方式、电池、Trainer |
| **7** | **Flags 7**：Mapper 高 4 位、VS/Playchoice、NES 2.0 标识 |
| **8** | **Flags 8**：PRG RAM 大小（罕见扩展） |
| **9** | **Flags 9**：电视制式（罕见扩展） |
| **10** | **Flags 10**：电视制式、PRG RAM 存在标志（非官方，极罕见） |
| **11-15** | 填充位（应为 0，但某些工具会在此写入标识，如 "DiskDude\!"） |

## **3\. 标志位详解 (Flag Definitions)**

### **Flags 6 (字节 6\)**

76543210  
||||||||  
|||||||+- 命名表镜像 (0: 横向排列/垂直镜像; 1: 纵向排列/水平镜像)  
||||||+-- 1: 含有电池备份的 PRG RAM ($6000-7FFF) 或其他持久存储  
|||||+--- 1: 在 $7000-$71FF 包含 512 字节的 Trainer  
||||+---- 1: 替代命名表布局 (4-Screen)  
\++++----- Mapper 编号的低 4 位 (Lower Nybble)

### **Flags 7 (字节 7\)**

76543210  
||||||||  
|||||||+- VS Unisystem  
||||||+-- PlayChoice-10 (CHR 数据后有 8 KB 的提示屏数据)  
||||++--- 如果等于 2，则表示使用 NES 2.0 格式 (字节 8-15 遵循新规范)  
\++++----- Mapper 编号的高 4 位 (Upper Nybble)

### **Flags 8 (字节 8\)**

* **PRG RAM 大小**：以 8 KB 为单位。值为 0 通常为了兼容性推断为 8 KB。现代用法建议改用 NES 2.0。

### **Flags 9 (字节 9\)**

* **Bit 0**：电视制式（0: NTSC; 1: PAL）。绝大多数模拟器忽略此位。

## **4\. 核心概念解析**

### **命名表镜像与 4-Screen**

* **位 0**：决定了物理内存 CIRAM A10 是连接到 PPU A10 还是 A11。这影响了屏幕翻页时的滚动方向。  
* **位 3 (4-Screen)**：如果置 1，表示板卡拥有额外的 VRAM 来同时存储 4 个命名表（如《雷霆赛车 2》）。这会覆盖位 0 的设置。

### **Mapper (存储器映射器)**

iNES 格式通过 Mapper 编号来区分不同的硬件板卡。

* **iNES 1.0**：支持 0-255 号 Mapper。  
* **NES 2.0**：支持 0-4095 号 Mapper。

### **Trainer**

Trainer 是一段 512 字节的代码，最初用于早期为了让游戏在 RAM 卡上运行而进行的指令转换或 CHR 缓存。在现代的原始卡带转储（Dump）中通常不应包含此段。

## **5\. 版本变体对比**

| 特性 | 早期 iNES (Archaic) | 标准 iNES (0.7+) | NES 2.0 |
| :---- | :---- | :---- | :---- |
| **Mapper 范围** | 0 \- 15 | 0 \- 255 | 0 \- 4095 |
| **字节 7** | 未使用 | Mapper 高位, Vs. | Mapper 高位, NES 2.0 签名 |
| **字节 8** | 未使用 | PRG RAM 大小 | Mapper 更高位, Submapper |
| **字节 9** | 未使用 | 电视制式 | ROM 大小高位扩展 |
| **字节 10** | 未使用 | 未使用 | PRG RAM (对数表示, 区分电池) |
| **字节 11** | 未使用 | 未使用 | VRAM 大小 (对数表示) |

## **6\. 模拟器识别逻辑 (推荐方案)**

为了正确解析 .nes 文件，模拟器应遵循以下探测顺序：

1. **检查 NES 2.0**：如果 (字节 7 AND $0C) \== $08，则判定为 **NES 2.0**。  
2. **检查早期 iNES**：如果 (字节 7 AND $0C) \== $04，判定为 **Archaic iNES**（通常忽略字节 7-15 的高位，因为可能包含 "DiskDude\!" 等垃圾字符）。  
3. **检查标准 iNES**：如果 (字节 7 AND $0C) \== $00 且 字节 12-15 均为 0，判定为 **标准 iNES**。  
4. **否则**：判定为 **iNES 0.7** 或带有冗余信息的旧格式。

**注意**：如果字节 12-15 包含非零值（且非 NES 2.0），某些模拟器会选择忽略 Mapper 的高 4 位，以防止因 "DiskDude\!" 字符串导致的 Mapper 编号计算错误（例如会将 Mapper 0 错误识别为 Mapper 64）。

[image1]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAFMAAAAZCAYAAABNcRIKAAAEM0lEQVR4Xu1XS2hTURBNaAT/H6TENklvUqJdKGqJKCJuxA8iFVFQoeLGjYiICLYouhBxIbiQIgpFEBeioiiCpS6y8FOwiuim4KZCkbqxSEEsSG2r5+TNxNub91IjXQR9B4Z378y58+bOm/t5kUiIECFChAgR4h9DLpebkQbQjLo2G6AsTCaTS/Gc6doI+jHG1CUSicWuzUIN7Eny2HaNfgD3ZNA7qwIMENIL+Qb5CXlaW1s71+URsA1B8kwCJrUE7YeQjNobGhouQ/+cdvYzmcxa+oS+LWJ9oFQqtRO8baKrAecQ5IfaXfDjwMeNcrFVBZhMThYTXFMumajELBLwWqsNvBOSqJxy0P9CHXjHRRUVn4NGkt7Y2LgA7V4dY/GuIPlxR18A/O2DfTQotqpEmWTGoL8NaVUFqwv9ca1CAv0R9WHp2B/RpCNhq9D/qnYF7C32h7EB/hsk9GJAbNWJoGRiku20sXK45PRpc/xAnvi8jm6MOlR4Av0BSD9kE3Wsdla9cmyAs4fVzGT7xeYH4V/zi1FWxj0/WxlEMW4ZCmhrPB6fQwX8b0HMu+rr61MuuYCAZHIJ3qKNg5kIOHqC5yiexyyeiyi3D/D6kFTjo+e7uE0s5z6LdrfFKYDjYH/FdiXJJMDNQzrtpEkiu6m3uVPBeKvyLeeCOD7iuR3yGHIfMuTyC5AJTgqYbepog6NnuqzRP0NdxDpYFEjOftjeQcay2ex81y6nPd9VEPi9gy8826HFWF3wdYqdSpMpiSskVHVGEllJVcoNhzHUIIac8baycbQXiX/moBQyucBk2vtjU1PTPOheQC6ozoVMiD4v2RNAfwcCPCcB0V4Q8i3OSyyrjdqvNJkK+sS77mJsHlvMLNc+FTDurNVulVi75Iaxl3Ox+UUIMTCZTKCPPq/7iB/E5wRkt/SbIcMRqWgs5ZXovxfeURnGreW8coi/TSaBcf3cSlx9pYCfDsaJWNpdWwlkQiUBQ3eatoAkK58JGIf0GO8irmOLJ7w9Ru0KOZgKHAS7mXuTLca7B9P/IPvueD/oUmcVSbuiJe7CeFe/Acbq2kpgT9rWc3LQT/B0Vp21zDvYl0RxPL9ci/I4TvRd5ZIptq50wB8Oq8EvtiBM156J927gKc62zkNjZFIDK17IJQHrRRtOV6hOfgV5GV/PPl8g44chzcoTHW8Ch6XPpTKidgV/CszvZV6CSpNppuE0l6vdB8R+M+LdtTmX4hkhMfHa54FEnbAjxYu2gMuYfyF9kE7IBMZetOx0flA4n8TvZ0iHu6fCttp4J+IjCtpjkAc2RwF9ncQzSVyeDfhfV676GA/vja7eBzHOkdVnvMI5AvluvBz0QJ92B/wxeLk23n90Z9BllYEa7x52FYk64NoV/FBp70Rvk4mVXLGqBZhLHVce27Id8Uyo2nhDhAgRIsR/hF+eaoBfJn7VDAAAAABJRU5ErkJggg==>

[image2]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEkAAAAZCAYAAAB9/QMrAAAD10lEQVR4Xu1YTWgTQRROSAXF3yo11qSZJA324A/WiNCTF0VE8KAV8dyDl+pBaQURqUdRQTxYKBUpUvxBD1KkSHsIvVjstViRCirFgh4K0oqtP/V72fe2k9fNmoBIMPvBx+588+bNmzezs7MbCgUIECBAgAD/OyLJZHK/Maae7nWljWw2uwKXsNYthOFnT2Nj4+ZQEV91dXVrUqnUvoaGhh26ruKAII9iQNNI0CGWIii3gefFBnUrUe4AZ8FFIg1S6gXQj4M/wV4qp9Pp9bgfBpvFhrWhaDS6msqxWGwT+1wAj4ldRQGBjYLtSqaVMIGZjlJBkoSE7sX1kk+SZsDpRCKRtrQWcCAej6+iMnx1gSNLrfLaCCdq3NYrBgjsi9cMQpvDYLNah3bBJ0k00DHY1Fpavfii1WOclUV2KbFBkk6ztihaRQGBveMAJ0O8z9AjgEF147am0Lp4kqjMfnJ2nehIRB+VcX8YfM37Wh6slZQk2AxxbMvAj3KP7dsPWN0Z+GoNLd83I5QDtwSjTgkQvI3ydlr+1KHVyEW5SWpqalrL+rBtb0N8gvO6TkP2OZ0I1ge17gf0+wZjfagXBPzcoHhcQ3IK4ToHmScaPXANFIolCajh9gVJwozERbdsC4BAX7LNgK7zgl4xkiDStG0xwLYZe67h+N4b561O46vF/Rj41TY+As4i0Cu4XuNgicNeq8knSeSrB/wFXyepzBPQz/5yyjwPOgbQjNLS13V+kBVFKwHXXnkxlIJMJrMOfd7FbZiu9DKROmt/fOw2QGEGhjepAZUR9C5oE2yo33q+SaKNmTpF/Tfu7AV4kX31a3uaSUoQzaauKwVITAx+J2n2dV0pQNsUOGWf1Th+ivdcXuB9pODxEBhnQ89p3S9JXuDlPA+22Lpx3nL35LzE2i3bxg/cPv/IGX7UytmPCGjTS2NR2hyNnSYgL/whSTnwmdb9kgT9CTiGDnaKBvsDug8aDPRue1BJ5yz2z/YkAo9RJ2nROI/a0pvdFDkPQf9oynzcyBfVkQ2VaRAY/COUD4oNa13gKeitFs+AH5a8eeNvvt04Sd+lzKvSjd8Fgt2NiknjfE70wOAprj+gb1F2feTAg3Nik3COE/QSoM10HFyAdlbqZeV6+BDmxLYY4O+yXyLQ91UkbJvWvYAVvxH+nqPfKfAz+Ap8K18aGhFaTcZJUmepnXggTG05WW0FB7LKBX2C1SO5G3BtB++HPA7RVQk6Mhhr/+THddTeGqoexvkUukPJQWJOGOcvxKC2q2rwyvnEey39Auoo50BaNcAhcmvS+Y+mP3ADlIPfoZJWG0aRV9MAAAAASUVORK5CYII=>