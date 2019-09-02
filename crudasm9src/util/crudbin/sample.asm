bits 16
mov ax,0x1234
mov cx,0xabcd
push byte 2
push word 0x1701
fstcw [bx]
fnstcw [si]
ret
lar si,di
lsl di,si
bt word [bx],ax
bt word [bx],byte 3
wait