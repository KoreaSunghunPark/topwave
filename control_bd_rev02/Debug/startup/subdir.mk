################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
../startup/startup_stm32f103xe.s 

OBJS += \
./startup/startup_stm32f103xe.o 

S_DEPS += \
./startup/startup_stm32f103xe.d 


# Each subdirectory must supply rules for building sources it contributes
startup/startup_stm32f103xe.o: ../startup/startup_stm32f103xe.s startup/subdir.mk
	arm-none-eabi-gcc -c -mcpu=cortex-m3 -g3 -c -Wa,-W -x assembler-with-cpp -MMD -MP -MF"startup/startup_stm32f103xe.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@" "$<"

