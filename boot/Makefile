targets	:= vmlinux.bin

$(obj)/vmlinux.bin: vmlinux FORCE
	$(call if_changed,objcopy)
	@echo 'Kernel: $@ is ready' ' (#'`cat .version`')'

clean-files += vmlinux.bin
