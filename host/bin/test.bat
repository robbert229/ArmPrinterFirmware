@ARMStrongHostOld.exe -i ./test.gcode -o ./test_old.cfcode -c -v > NUL
@ARMStrongHost.exe -i ./test.gcode -o ./test.cfcode -c -v > NUL
FC /B test.cfcode test_old.cfcode

@ARMStrongHostOld.exe -i ./test.gcode -o ./test_old.fcode -v > NUL
@ARMStrongHost.exe -i ./test.gcode -o ./test.fcode -v > NUL
FC /B test.fcode test_old.fcode
