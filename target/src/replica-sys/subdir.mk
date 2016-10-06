# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/replica-sys/replica.c 

OBJS += \
./src/replica-sys/replica.o 


# Each subdirectory must supply rules for building sources it contributes
src/replica-sys/%.o: ../src/replica-sys/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -fPIC -rdynamic -std=gnu99 -DDEBUG=$(DEBUGOPT) -O0 -g3 -Wall -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


