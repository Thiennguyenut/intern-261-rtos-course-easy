# Dự đoán — Week 1 / Ex1

> **Commit file này TRƯỚC khi nạp chương trình lần đầu.** Report sẽ phải giải thích
> mọi chỗ dự đoán lệch so đo. Dự đoán sai không bị trừ điểm; dự đoán sau khi đo thì bị.

`student_id`: 2413284    Ngày/giờ commit: 12h38 ngày 15/10/2026

1. Chu kỳ LED tôi sẽ đo được: 112 ms (`BLINK_PERIOD_MS` = 113 ms)
2. Sai số giữa chu kỳ đo được và `BLINK_PERIOD_MS`: 1000 us. Vì sao?
- vTaskDelay() chờ theo tick nguyên, mà mỗi lần LED đổi trạng thái là nửa chu kỳ ứng với 56,5 ms, theo cấu hình configTICK_RATE_HZ = 1000 thì 1 tick = 1ms, do đó sẽ có thể lấy 56 ms. Vậy sai số giữa chu kỳ đo được và `BLINK_PERIOD_MS` là 0,5 ms * 2 = 1 ms = 1000us
- Ngoài ra thời điểm gọi vTaskDelay có thể nằm giữa hai tick, thao tác đổi trạng thái LED, xử lý ngắt và chuyển context cần thời gian thực thi.
3. Nếu để HAL timebase ở lại SysTick, điều gì sẽ xảy ra khi `vTaskDelay()` được gọi?
   (chọn và giải thích: chạy bình thường / treo / tick sai nhịp / hard fault)
- Khi để HAL timebase ở lại SysTick thì có thể sẽ bị tick sai nhịp.
- Vì khi giao SysTick cho FreeRTOS, handler mới không còn gọi HAL_IncTick(). Khi đó, tick FreeRTOS vẫn được cập nhật nên vTaskDelay() có thể hoạt động bình thường. Tuy nhiên, tick HAL sẽ ngừng tăng hoặc tăng khi SysTick được giao trở lại cho  HAL, do đó sẽ khiến các hàm dựa vào thời gian HAL như HAL_Delay() có thể bị kẹt hoặc tick sai nhịp
4. `BRINGUP_STACK_WORDS` = 112 words = 448 bytes. Task này có đủ không? Vì sao?
- Em dự đoán đủ. Vì task hiện tại chỉ điều khiển GPIO và gọi vTaskDelay(), không dùng đệ quy, mảng cục bộ lớn nên không cần quá nhiều dung lượng cho stack.
5. Trước khi map handler, linker sẽ báo lỗi gì (đoạn nguyên văn thông báo):
- Có thể linker sẽ báo lỗi **multiple definition**, vì nếu map tên handler của FreeRTOS sang tên trong vector table nhưng vẫn giữ các hàm cùng tên do CubeMX sinh. Ví dụ như vài hàm từ stm32f1xx_it.c và từ port FreeRTOS trùng nhau sau khi map nên linker báo lỗi