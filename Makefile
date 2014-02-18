build:
	$(MAKE) -C quicklz
	$(MAKE) -C fio
	$(MAKE) -C host
	$(MAKE) -C client

clean:
	$(MAKE) -C quicklz clean
	$(MAKE) -C fio clean
	$(MAKE) -C host clean
	$(MAKE) -C client clean