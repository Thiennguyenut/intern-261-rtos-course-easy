# Report — Week 4 / Ex1 (Design doc)

`student_id`: 2413284   Commit của PREDICTION.md: b8ad46d2221b4bd7fae0ce7cc804e0039e32d9f1

> File này ĐÓNG VAI TRÒ design doc 2–3 trang nộp cho mentor duyệt trước khi
> viết code. Điền đủ số liệu — không mô tả chung chung.

## 1. Task diagram
Mô tả (sơ đồ ASCII hoặc ảnh chèn vào) luồng: ISR lấy mẫu MPU6050 → hàng đợi
→ task xử lý → task ghi UART. Ghi rõ priority từng task.
**Trả lời:**

```text
TIM2 ISR, 400 Hz (NVIC 7)
  │ đọc MPU6050 14 byte qua I2C1 400 kHz (khoảng 390 us, timeout 2 ms)
  │ t0: mẫu đã nằm trong RAM, ghi timestamp DWT->CYCCNT
  │ xQueueSendFromISR + portYIELD_FROM_ISR
  ▼
sample_queue (16 mẫu x 16 B)
  ▼
proc_task (priority 5)
  │ t1: nhận mẫu → lọc trung bình 4 mẫu → ghi vào block A/B
  │ t2: xử lý xong. Deadline: t2 − t0 ≤ 300 us
  │ đủ 64 mẫu: xTaskNotify(logger_task, index block)
  ▼
logger_task (priority 2)
  ▲
  │ thêm header 8 B → HAL_UART_Transmit_DMA 1032 B, 115200 baud
  │ chờ uart_done
  ▼
USART1 TC ISR (NVIC 8): xSemaphoreGiveFromISR(uart_done)
```
**Priority và lý do** (deadline càng ngắn thì priority càng cao):

| Thành phần | Priority | Lý do |
|---|---|---|
| TIM4 (HAL tick) | NVIC 6 | Cao hơn TIM2 để `HAL_GetTick()` vẫn tăng khi đang trong ISR lấy mẫu, timeout I2C mới hoạt động |
| TIM2 ISR | NVIC 7 | ≥ 5 (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`) mới gọi được FromISR. Mức đã đo latency 16,32 us ở week3/ex1 |
| DMA1_Channel4, USART1 | NVIC 8 | Thấp hơn TIM2 để không làm lệch thời điểm lấy mẫu, logger có 160 ms nên chờ thêm không sao |
| proc_task | 5 | Deadline ngắn nhất (300 us), phải chạy ngay sau ISR |
| logger_task | 2 | Deadline 160 ms/khối, gửi bằng DMA nên CPU rảnh khi truyền |
| Idle | 0 | Mặc định |

- Nếu đảo ngược (logger_task > proc_task) và gửi polling, proc_task phải chờ tới
89,58 ms, queue 16 mẫu tràn. Priority 1, 3, 4 để dành cho measure_task và
noise_task ở ex3, nên `configMAX_PRIORITIES = 6`.

## 2. Timing budget
| Đại lượng | Công thức / nguồn số liệu | Giá trị |
|---|---|---|
| Chu kỳ lấy mẫu | 1 / `SAMPLE_RATE_HZ` | 2500 us |
| Latency ISR→task (đo thật, week3/ex1) | [week3/ex1 mục 2](../../week3/ex1/REPORT.md) | 16,32 us |
| Thời gian xử lý 1 mẫu (ước lượng) | Chép mẫu vào và ra queue 20 + Lọc 20 + ghi block 5 + báo logger 20 | 65 us |
| Dự phòng worst-case | Ngắt chen vào (SysTick 15 + TIM4 10 + UART/DMA 30) 55 + kernel khóa ngắt 20 | 75 us |
| Tổng so với `ISR_TO_PROC_DEADLINE_US` | 16,32 + 65 + 75 = 156,32 us < `ISR_TO_PROC_DEADLINE_US = 300 us` | Đạt |

## 3. RAM budget
| Thành phần | Kích thước | Ghi chú |
|---|---|---|
| Queue (độ dài × cỡ phần tử) | 16 phần tử × 16 bytes = 256 bytes, `Queue_t` 80 bytes và 8 bytes header nên tổng 344 bytes | `sample_queue` chứa mẫu thô từ ISR. Chịu được proc_task dừng khoảng 40 ms |
| Stack task lấy mẫu | 0 bytes | Lấy mẫu bằng Timer ISR, dùng MSP 1024 bytes, không cấp phát từ heap |
| Stack task xử lý | stack riêng 256 words = 1024 bytes + 8 bytes header; TCB 96 bytes + 8 bytes header nên tổng 1136 bytes | |
| Stack task ghi UART | stack riêng 256 words = 1024 bytes  + 8 bytes header; TCB 96 bytes + 8 bytes header nên tổng 1136 bytes | |
| Stack IDLE task | stack riêng 128 words = 512 bytes  + 8 bytes header; TCB 96 bytes + 8 bytes header nên tổng 624 bytes | |
| Binary semaphore `uart_done` | `Queue_t` 80 bytes + 8 bytes header nên tổng 88 bytes | Báo DMA gửi xong cho logger_task |
| Căn chỉnh và end marker heap_4 | 16 bytes | Theo week1/ex4 |
| **Tổng** | 344 + 0 +  1136 + 1136 + 624 + 88 + 16 = **3344 bytes** | So với `MAX_HEAP_BYTES` thì nhỏ hơn, còn dư khoảng 11264 − 3344 = **7920 bytes** nên đạt |

## 4. Lựa chọn API đồng bộ
Queue / event group / stream buffer — vì sao chọn cái này cho pipeline này.
**Trả lời:**
| Đoạn | API | Lý do |
|---|---|---|
| ISR → proc_task | Queue | Chép nguyên struct 16 B, giữ FIFO, `xQueueSendFromISR` báo được khi đầy |
| proc_task → logger_task | Block A/B + task notification | proc_task ghi thẳng vào block, chỉ báo index (0 hoặc 1) khi đủ 64 mẫu |
| USART1 ISR → logger_task | Binary semaphore | Một sự kiện "gửi xong", tách riêng với notification để không lẫn với index block |

**Dùng block A/B thay cho queue thứ hai sau proc_task:**
- DMA cần một buffer liền nhau 1032 B để gửi một lần, nên dù dùng queue thì logger
  vẫn phải có block. Ghi thẳng vào block thì mỗi mẫu chỉ chép 1 lần thay vì 2.
- Đường deadline không phải gọi `xQueueSend` cho từng mẫu, chỉ mẫu thứ 64 mới
  gọi `xTaskNotify`.
- Hai block cho phép proc_task ghi khối mới trong lúc DMA gửi khối cũ.
- Đánh đổi: code phức tạp hơn vì phải tự quản lý cờ bận của 2 block. Đổi lại,
  RAM 2064 B ít hơn cách queue 64 mẫu + buffer gửi (2136 B).

**Không dùng event group:** vì nó không mang dữ liệu mẫu, và nhiều lần set cùng
một bit trước khi task kịp đọc sẽ bị gộp thành một, làm mất số lần lấy mẫu.

**Không dùng stream buffer:** vì nó truyền luồng byte, phải tự đếm và ghép đủ
16 B thành một mẫu.
## 5. Xử lý khi hàng đợi đầy
Drop mẫu mới hay mẫu cũ? Có đếm số mẫu bị rớt để báo cáo ở ex3 không?
**Trả lời:**
- **sample_queue đầy: drop mẫu mới.**
    - `xQueueSendFromISR` trả về `errQUEUE_FULL`, ISR bỏ mẫu vừa đọc và tăng
    `drop_sample_count`. ISR không chờ, 16 mẫu cũ trong queue giữ nguyên thứ tự FIFO.
    - Lấy t = 0 là lúc start TIM2 và giả sử proc_task ngừng nhận mẫu: mẫu thứ 16 làm
    queue đầy tại 16 x 2,5 + 0,39 = 40,39 ms. Mẫu thứ 17 đọc xong tại 42,89 ms và
    bị bỏ.
    - Chọn drop mẫu mới vì queue dài hơn 1 không ghi đè được mẫu cũ trong ISR, và
    giữ được một đoạn dữ liệu liên tục trước khoảng bị mất.

- **Cả hai block UART đều bận:** bỏ block vừa đầy, tăng `block_drop_count`, không
ghi đè block đang gửi bằng DMA. Ngoài ra em nghĩ bình thường sẽ không xảy ra vì 89,58 ms < 160 ms.
- **Có đếm để báo cáo ở ex3:** `drop_sample_count` (mẫu rớt ở queue), `block_drop_count`
  (khối rớt), mức đầy cao nhất của queue qua `uxQueueMessagesWaiting`.

## 6. Mentor duyệt
Chữ ký / ghi chú của mentor sau buổi duyệt thiết kế: ______
