################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/dare/dare_ibv.c \
../src/dare/dare_ibv_ud.c \
../src/dare/dare_ep_db.c \
../src/dare/dare_ibv_rc.c \
../src/dare/dare_server.c

OBJS += \
./src/dare/dare_ibv.o \
./src/dare/dare_ibv_ud.o \
./src/dare/dare_ep_db.o \
./src/dare/dare_ibv_rc.o \
./src/dare/dare_server.o \


# Each subdirectory must supply rules for building sources it contributes
src/dare/%.o: ../src/dare/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -fPIC -rdynamic -std=gnu99 -DDEBUG=$(DEBUGOPT) -O0 -g3 -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


