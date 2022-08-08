################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Kernel.c \
../src/KernelUtils.c \
../src/Planificador.c 

OBJS += \
./src/Kernel.o \
./src/KernelUtils.o \
./src/Planificador.o 

C_DEPS += \
./src/Kernel.d \
./src/KernelUtils.d \
./src/Planificador.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/workspace/tp-2022-1c-scalonetaSO/Utils/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


