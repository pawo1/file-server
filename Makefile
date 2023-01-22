.PHONY: clean All

All:
	@echo "----------Building project:[ server - Debug ]----------"
	@cd "server" && "$(MAKE)" -f  "server.mk"
clean:
	@echo "----------Cleaning project:[ server - Debug ]----------"
	@cd "server" && "$(MAKE)" -f  "server.mk" clean
