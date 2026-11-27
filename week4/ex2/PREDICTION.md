# Dự đoán — Week 4 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284

1. Lỗi gọi API queue không phải `FromISR` bên trong ISR sẽ gây ra hiện
   tượng gì khi chạy (configASSERT? treo? reset?). Giải thích bằng cơ chế
   (ngăn xếp ISR, gọi `vTaskSuspendAll`/scheduler từ ngữ cảnh ngắt), không
   chỉ "nó bị lỗi".
   **Trả lời:**
   - Dự đoán: configASSERT kích hoạt, board treo, không reset.
   - Vì ISR chạy trên MSP và không có TCB, nên trong ISR `pxCurrentTCB` vẫn là task bị ngắt. `xQueueSend` là API cho task: khi queue đầy nó gọi `vTaskSuspendAll` rồi `vTaskPlaceOnEventList`, đưa `pxCurrentTCB` vào danh sách chờ. Gọi từ ISR thì kernel sẽ khóa scheduler và block nhầm task bị ngắt, còn ISR vẫn chạy tiếp. PendSV (priority 15) cũng không đổi task được khi TIM2 đang chạy, và SysTick không chạy nên timeout không bao giờ hết.
   - Kernel chặn việc đó bằng cách xQueueSend gọi taskENTER_CRITICAL → vPortEnterCritical, hàm này kiểm tra VECTACTIVE trong ICSR. Trong TIM2 ISR thì VECTACTIVE ≠ 0, nên assert kích hoạt trước khi kịp tới `vTaskSuspendAll`. Do đó assert gọi `portDISABLE_INTERRUPTS()` + `for(;;)` nên board bị treo ngay, sẽ không reset vì không bật IWDG.

2. Sau khi sửa đúng API `FromISR`, dự đoán số mẫu ghi UART mỗi giây =
   `SAMPLE_RATE_HZ` / `LOG_BLOCK_SAMPLES` khối/giây. Tính ra con số.
   **Trả lời:**
   - Số khối/giây = `SAMPLE_RATE_HZ` / `LOG_BLOCK_SAMPLES` = 400 / 64 = **6,25 khối/s**, tức mỗi khối cách nhau
     64 / 400 = **160 ms**.
   - Số mẫu/giây = 6,25 x 64 = **400 mẫu/s**.
   - Số byte/giây = 6,25 x 1032 = **6450 Byte/s**, tương đương
     6450 / 11520 = **56,0%** băng thông UART nên vẫn truyền được.
   
3. Với `UART_BAUD` của bạn, một khối `LOG_BLOCK_SAMPLES` mẫu mất bao lâu để
   truyền xong? Có nguy cơ UART chưa gửi xong khối cũ mà khối mới đã sẵn
   sàng không?
   **Trả lời:**
   - Một khối có 1032 B (header 8 B + 64 mẫu x 16 B). 8N1 nên mỗi byte 10 bit:
     1032 x 10 / 115200 = **89,58 ms**. Vì khối mới sẵn sàng gửi sau 160 ms, nên UART gửi xong khối cũ sớm hơn **70,42 ms**. Dự đoán không có nguy cơ chồng khối, `block_drop_count` = 0.
   - Ngoài ra có 2 block A/B nên ngay cả khi DMA còn gửi block A, proc_task
     vẫn ghi được vào block B.
