################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/drivers/anim.c \
../Core/Src/drivers/buzzer.c \
../Core/Src/drivers/eyes.c \
../Core/Src/drivers/lcd_gfx.c \
../Core/Src/drivers/lcd_st7735.c \
../Core/Src/drivers/motor.c \
../Core/Src/drivers/rgb_led.c \
../Core/Src/drivers/servo.c \
../Core/Src/drivers/ultrasonic.c 

OBJS += \
./Core/Src/drivers/anim.o \
./Core/Src/drivers/buzzer.o \
./Core/Src/drivers/eyes.o \
./Core/Src/drivers/lcd_gfx.o \
./Core/Src/drivers/lcd_st7735.o \
./Core/Src/drivers/motor.o \
./Core/Src/drivers/rgb_led.o \
./Core/Src/drivers/servo.o \
./Core/Src/drivers/ultrasonic.o 

C_DEPS += \
./Core/Src/drivers/anim.d \
./Core/Src/drivers/buzzer.d \
./Core/Src/drivers/eyes.d \
./Core/Src/drivers/lcd_gfx.d \
./Core/Src/drivers/lcd_st7735.d \
./Core/Src/drivers/motor.d \
./Core/Src/drivers/rgb_led.d \
./Core/Src/drivers/servo.d \
./Core/Src/drivers/ultrasonic.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/drivers/%.o Core/Src/drivers/%.su Core/Src/drivers/%.cyclo: ../Core/Src/drivers/%.c Core/Src/drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-drivers

clean-Core-2f-Src-2f-drivers:
	-$(RM) ./Core/Src/drivers/anim.cyclo ./Core/Src/drivers/anim.d ./Core/Src/drivers/anim.o ./Core/Src/drivers/anim.su ./Core/Src/drivers/buzzer.cyclo ./Core/Src/drivers/buzzer.d ./Core/Src/drivers/buzzer.o ./Core/Src/drivers/buzzer.su ./Core/Src/drivers/eyes.cyclo ./Core/Src/drivers/eyes.d ./Core/Src/drivers/eyes.o ./Core/Src/drivers/eyes.su ./Core/Src/drivers/lcd_gfx.cyclo ./Core/Src/drivers/lcd_gfx.d ./Core/Src/drivers/lcd_gfx.o ./Core/Src/drivers/lcd_gfx.su ./Core/Src/drivers/lcd_st7735.cyclo ./Core/Src/drivers/lcd_st7735.d ./Core/Src/drivers/lcd_st7735.o ./Core/Src/drivers/lcd_st7735.su ./Core/Src/drivers/motor.cyclo ./Core/Src/drivers/motor.d ./Core/Src/drivers/motor.o ./Core/Src/drivers/motor.su ./Core/Src/drivers/rgb_led.cyclo ./Core/Src/drivers/rgb_led.d ./Core/Src/drivers/rgb_led.o ./Core/Src/drivers/rgb_led.su ./Core/Src/drivers/servo.cyclo ./Core/Src/drivers/servo.d ./Core/Src/drivers/servo.o ./Core/Src/drivers/servo.su ./Core/Src/drivers/ultrasonic.cyclo ./Core/Src/drivers/ultrasonic.d ./Core/Src/drivers/ultrasonic.o ./Core/Src/drivers/ultrasonic.su

.PHONY: clean-Core-2f-Src-2f-drivers

