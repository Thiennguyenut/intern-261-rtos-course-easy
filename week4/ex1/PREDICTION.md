# Dự đoán — Week 4 / Ex1

> **Commit TRƯỚC khi gửi design doc cho mentor duyệt.**

`student_id`: 2413284

1. Với `SAMPLE_RATE_HZ` của bạn, chu kỳ lấy mẫu là bao nhiêu us? Dựa trên
   latency ISR→task thật bạn đã đo ở `week3/ex1`, dự đoán task xử lý có kịp
   `ISR_TO_PROC_DEADLINE_US` không? Trình bày phép tính, không đoán.
   **Trả lời:**
   - Với `SAMPLE_RATE_HZ` = 400 Hz thì T = 1 / 400 Hz = 2,5 ms = 2500 us.
   - Latency ISR→task đo ở week3/ex1 = 16,32 us.
   - `ISR_TO_PROC_DEADLINE_US` = 300 us, tính từ t0 (lúc ISR đã đọc dữ liệu vào RAM) đến t2 (proc_task xử lý xong mẫu).
   - Nếu đọc I2C trong đường deadline: 153 xung SCL / 400 kHz = 382,5 us. Chỉ riêng I2C + latency đã là 382,5 + 16,32 = 398,82 us > 300 us. Vì vậy I2C phải nằm ngoài đường deadline.
   - Thiết kế em chọn là:
     T_total = latency ISR→task + T_copy_in/out_Queue + T_process + B_irq + B_crit = 16,32 + 20 + 45 + 55 + 20 = 156,32 us.
   - Dự đoán ĐẠT, dư khoảng 143,68 us.
   
2. Dự đoán tổng RAM (queue + stack mọi task) theo thiết kế của bạn, tính
   bằng bytes. So với `MAX_HEAP_BYTES`: còn dư bao nhiêu?
   **Trả lời:**
   - Heap_4 cấp mỗi khối = kích thước yêu cầu + 8 bytes header, căn theo 8 bytes.
     Theo cấu hình FreeRTOSConfig thì TCB = 96 bytes, Queue_t = 80 bytes

   | Thành phần  | Phép tính | Bytes |
   |---|---|---|
   | sample_queue | 16 x 16 + 80 + 8 | 344 |
   | uart_done (binary semaphore) | 80 + 8 | 88 |
   | Stack proc_task | 256 words x 4 + 8 | 1032 |
   | Stack logger_task | 256 words x 4 + 8 | 1032 |
   | Stack Idle | 128 words x 4 + 8 | 520 |
   | 3 TCB | 3 x (96 + 8) | 312 |
   | Căn chỉnh và end marker của heap_4 | Theo week1/ex4 | 16 |
   | **Tổng heap** | | **3344** |

   - So với `MAX_HEAP_BYTES` = 11264 bytes: còn dư 11264 − 3344 = 7920 bytes.
   - Buffer tĩnh ngoài heap: 2 block UART 2064 + text stats 512 + bộ lọc 80
     + bộ đếm 40 + handle TIM/I2C/DMA 256 = 2952 bytes.
   - Thêm text stats 512 bytes ở vùng tĩnh, SRAM còn 1904 bytes.
   
3. Nếu task xử lý tạm thời chậm hơn ISR lấy mẫu (ví dụ do một task khác
   chiếm CPU), hàng đợi của bạn đầy sau bao nhiêu mẫu? Điều gì xảy ra với
   mẫu tiếp theo trong thiết kế của bạn?
   **Trả lời:**
   - Queue dài 16 mẫu với chu kỳ lấy mẫu 2,5 ms, phần đọc I2C khoảng 390 us.
   - Giả sử queue rỗng và proc_task ngừng nhận mẫu. Chọn t = 0 là lúc start TIM2. Ngắt đầu tiên xảy ra sau một chu kỳ, nên TIM2 vào ISR lần thứ N tại N x 2,5 ms và mẫu thứ N vào queue tại N x 2,5 ms + 0,39 ms.
   - Mẫu thứ 16 làm queue đầy tại 16 x 2,5 + 0,39 = 40,39 ms.
   - Mẫu thứ 17: TIM2 vào ISR tại 17 x 2,5 = 42,5 ms, đọc I2C xong tại 42,89 ms. Lúc này `xQueueSendFromISR` trả về `errQUEUE_FULL`, ISR bỏ mẫu mới và tăng
   `drop_count`. ISR không chờ nên 16 mẫu cũ trong queue vẫn giữ nguyên thứ tự FIFO.
   
4. Bạn dự đoán priority nào (ISR task xử lý vs. task ghi UART) sẽ cho kết
   quả đạt deadline tốt hơn? Vì sao?
   **Trả lời:**
   - Em dự đoán proc_task có priority cao hơn logger_task sẽ đạt deadline tốt
     hơn. Thiết kế chọn proc_task = 5, logger_task = 2.
   - Một khối UART gồm 8 bytes header + 64 x 16 bytes = 1032 bytes. Ở 115200
     baud, 8N1: 1032 x 10 / 115200 = 89,58 ms.
   - Nếu logger_task cao hơn và gửi bằng polling, proc_task có thể phải chờ tới
     89,58 ms, gấp khoảng 299 lần deadline 300 us. Trong lúc đó có khoảng
     89,58 / 2,5 = 36 mẫu đến, queue chỉ giữ 16 mẫu nên rớt khoảng 20 mẫu.
   - Nếu proc_task cao hơn, sau khi ISR đưa mẫu vào queue, `portYIELD_FROM_ISR`
     chuyển thẳng sang proc_task, không phải chờ logger_task. Logger gửi bằng
     DMA nên CPU vẫn rảnh trong lúc UART truyền.
   - logger_task chỉ cần gửi xong mỗi khối trong 160 ms (Vì 1 khối 64 mẫu với tần số lấy mẫu 400 Hz), thời gian
     truyền 89,58 ms nên đạt.