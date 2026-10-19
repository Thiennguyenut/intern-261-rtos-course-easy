# Dự đoán — Week 1 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284    `HEAP_SIZE_BYTES` = 8192   `DYN_TASK_STACK_WORDS` = 160
`STATIC_TASK_STACK_WORDS` = 128   `HOG_DEPTH_BYTES` = 512   `TARGET_FREE_HEAP_BYTES` = 3072

1. Ngân sách RAM dự đoán (tổng phải = 20480 bytes):

| Mục | Bytes dự đoán |
|---|---|
| `.data` | 128 |
| `.bss` (không kể heap FreeRTOS) | 2560 |
| Heap FreeRTOS (`ucHeap`) | 8192 |
| Stack của main (MSP) | 1024 (Min_Stack_Size) |
| Còn lại / chưa dùng | 8576 |
| **Tổng** | **20480** |

2. `xPortGetFreeHeapSize()` ngay sau khi scheduler chạy: 6328 bytes.
   `HEAP_SIZE_BYTES` − số này = 1864 bytes. Số đó là của những gì?
**Trả lời:**
- Stack dynamic của dyn_task: 640 bytes.
- Stack dynamic của report task: 1024 bytes.
- Hai TCB: 2 × 76 = 152 bytes.
- Bốn allocation header: 32 bytes.
- Padding của hai block TCB: 8 bytes.
- End marker khi khởi tạo heap: 8 bytes.
3. High water mark dự đoán (words): dyn 120 words , stat 88 words , idle 96 words , timer 64 words .
4. `dyn_task` có stack 160 words = 640 bytes. Mảng `HOG_DEPTH_BYTES` = 512 bytes.
   Hook tràn stack **có** chạy không? Vì sao?
**Trả lời:**
- Hook tràn stack **có** chạy khi bật mảng HOG. Vì mảng thực sự được đặt trên stack, được ghi đầy đủ và phần dùng stack nền vẫn khoảng 40 words nên tổng stack cần = 512 + 40 × 4 = 672 bytes. Vì vượt stack được cấp 32 bytes nên khi task chuyển context, kiểm tra mức 2 dự kiến phát hiện stack pointer hoặc mẫu biên bị vi phạm rồi gọi overflow hook.
5. Heap trong `.map` và `xPortGetFreeHeapSize()` sẽ lệch nhau. Lệch vì: .map ghi nhận cả vùng ucHeap 8192 bytes được dành trong SRAM. API runtime trả phần còn trống sau các allocation và overhead allocator.
6. Stack nhỏ nhất an toàn của `dyn_task` dự đoán: 200 words. Dựa vào đâu?
**Trả lời:**
Mảng HOG:                  128 words
Context và phần dùng nền:   40 words
Biên an toàn:               32 words
Tổng:                      200 words

