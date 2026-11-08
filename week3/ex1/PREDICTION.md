# Dự đoán — Week 3 / Ex1

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284

1. Với `ISR_NVIC_PRIO` của bạn và nhóm priority `NVIC_PRIORITYGROUP_4`, số này
   nằm trong khoảng cho phép (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` ..
   `configLIBRARY_LOWEST_INTERRUPT_PRIORITY`) không? Trình bày phép so sánh.
   **Trả lời:**
   - Có nằm trong khoảng cho phép, vì với `ISR_NVIC_PRIO` = 7, `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` = 5, `configLIBRARY_LOWEST_INTERRUPT_PRIORITY` = 15 nên **5 ≤ 7 ≤ 15** nên giá trị của thanh ghi priority sau khi dịch trái 4 bit là 0x50 ≤ 0x70 ≤ 0xF0. Do đó EXTI priority 7 được phép gọi FreeRTOS API loại FromISR.

2. Khi đặt priority EXTI cao hơn ngưỡng cho phép rồi gọi
   `vTaskNotifyGiveFromISR`, bạn dự đoán hành vi quan sát được là gì (reset?
   treo? thông báo gì trên UART/debugger)?
   **Trả lời:**
   - Giả sử EXTI priority 4. Vì 4 < 5, EXTI có ưu tiên cao hơn ngưỡng được phép gọi API FreeRTOS. Khi nhấn nút và ISR gọi vTaskNotifyGiveFromISR, em dự đoán chương trình sẽ kích hoạt assert trong vPortValidateInterruptPriority. Điều kiện được kiểm tra là: ucCurrentPriority >= ucMaxSysCallPriority ứng với 0x40  >= 0x50 sẽ sai.
   - Em dự đoán chương trình sẽ bị treo trong vòng lặp vô hạn của configASSERT do đó PA8 sẽ không toggle trạng thái, không tự in UART và debugger dự kiến dừng tại assert trong vPortValidateInterruptPriority.

3. Dự đoán latency ISR → task (có `portYIELD_FROM_ISR`) theo đơn vị us, dựa
   trên tần số CPU 72 MHz và số lệnh ước lượng trong đường đi ISR → context
   switch. So với `LATENCY_BUDGET_US`: đạt hay không?
   **Trả lời:**
   - Đường xử lý gồm vào ngắt (~27 cycles), xử lý EXTI của HAL (~100 cycles), gửi notification (~250 cycles), yêu cầu PendSV (~40 cycles),chuyển EXTI sang PendSV (~ 13 cycles), chuyển context sang ack_task (~160 cycles), thoát PendSV (~18 cycles), trở về từ lệnh chờ và toggle PA8 (~60 cycles). Tổng 668 cycles xấp xỉ 9,27 us. Vì 9,27 us < LATENCY_BUDGET_US = 80 µs nên đạt theo yêu cầu.

4. Nếu bỏ `portYIELD_FROM_ISR`, `ack_task` sẽ chạy **khi nào** (ngay lập tức
   hay phải chờ tick ngắt kế tiếp)? Latency tăng thêm cỡ bao nhiêu?
   **Trả lời:**
   - Notification vẫn đưa ack_task từ Blocked sang Ready. Tuy nhiên, ISR không yêu cầu PendSV qua portYIELD_FROM_ISR, nên task vừa được đánh thức chưa được chuyển sang chạy ngay khi ISR kết thúc. Do đó em dự đoán ack_task sẽ chạy sau tick FreeRTOS kế tiếp.
   - Với tick 1.000 Hz, khoảng cách giữa hai tick là 1 ms nên latency tăng thêm sẽ phụ thuộc vào thời điểm nhấn nút, có thể từ 0 us - 1000 us
