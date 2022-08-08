################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/GestionPaginas.c \
../src/Memoria.c \
../src/MemoriaUtils.c \
../src/Swap.c 

OBJS += \
./src/GestionPaginas.o \
./src/Memoria.o \
./src/MemoriaUtils.o \
./src/Swap.o 

C_DEPS += \
./src/GestionPaginas.d \
./src/Memoria.d \
./src/MemoriaUtils.d \
./src/Swap.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/utnso/workspace/tp-2022-1c-scalonetaSO/Utils/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


