ansible agent -i tsinghua_host -m shell -a 'echo ==== {{inventory_hostname}} `ifconfig eth4|grep HWaddr|cut -d " " -f 11`' -u root -k