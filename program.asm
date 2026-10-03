; 8085 Assembly Program
; Add two numbers and store result

        MVI A, 05H
        MVI B, 03H
        ADD B
        STA 2050H
        HLT