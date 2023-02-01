.PHONY: clean All

All:
	@echo "----------Building project:[ server - Debug ]----------"
	@cd "server" && "$(MAKE)" -f  "server.mk"
	@echo "----------Building project:[ client - Debug ]----------"
	@cd "client" && "$(MAKE)" -f  "client.mk"
clean:
	@echo "----------Cleaning project:[ server - Debug ]----------"
	@cd "server" && "$(MAKE)" -f  "server.mk" clean
	@echo "----------Cleaning project:[ client - Debug ]----------"
	@cd "client" && "$(MAKE)" -f  "client.mk" clean
