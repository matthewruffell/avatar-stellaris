function buffer_symbolic_all (state, plg)
   print ("[S2E]: making buffer symbolic\n")
   buff = state:readRegister("r4") + 8 --r4 [#8] contains the buffer memory address
   length = state:readRegister("r4") + 4 -- r4 [#4] contains the symbolic length
   for i = 0,length do
      state:writeMemorySymb("VulnString", buff+i, 1)
   end
   -- Write null byte
   state:writeMemory(buff + length, 1, 0)
end

function return_symbolic (state, plg)
   print ("[S2E]: making returned symbolic\n")
   state:writeRegisterSymb("r0", "VulnString") -- r0 is the returned value from uart
end

function uart_symbolic (state, plg)
   print ("[S2E]: making uart read value symbolic")
   uart = state:readRegister("r0") -- should be hardware address for UART0 ie 0x4000C000
   state:writememorySymb("UARTSymb", uart, 1, 0, 50)
end

function vulnfunc_testcase (state, plg)
   print ("[S2E]: Generating testcase and terminating")
   plg:setGenerateTestcase(true)
   plg:generate_testcase_on_kill(false)
   plg:setKill(true)
end

function vulnfunc_symbolic (state, plg)
   buff = state:readRegister("r7") --r7 contains the buffer memory address
   for i = 0,25 do
      state:writeMemorySymb("VulnString", buff-i, 1)
   end
   -- Write null byte
   state:writeMemory(buff, 1, 0)
end
