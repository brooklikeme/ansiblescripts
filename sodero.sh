#!/bin/bash

ROOT="/root/sodero"
PLAYBOOK="tsinghua_host"
NODE="agent"

function doPing {
    ansible part_servers -i tsinghua_host -m ping -u root -k
}

function doCommand {
    ansible $NODE -i tsinghua_host -m command -a '$CMD' -u root -k
}

function doNtp {
    ansible $NODE -i tsinghua_host -m command -a 'ntpdate $NTP_SERVER' -u root -k
}

function loadNIC {
    ansible agent -i tsinghua_host -m command -a 'modprobe -r mlx4_en;modprobe mlx4_en' -u root -k
}

function getTCPSack {
    ansible agent -i tsinghua_host -a 'cat /proc/sys/net/ipv4/tcp_sack' -u root -k
}

function setTCPSack {
    ansible agent -i tsinghua_host -a '/sbin/sysctl -q -w net.ipv4.tcp_sack=1' -u root -k
}

function crossPing {
    ansible part_servers -i tsinghua_host -a 'ping 10.5.3.1 -c 2' -u root -k
}

function proxyPing {
    ansible-playbook -i tsinghua_host proxy_ping.yml -u root -k
}

function mlxInstall {
    ansible-playbook -i tsinghua_host mlx_install.yml -u root -k
}

function arpInstall {
    ansible-playbook -i tsinghua_host arp_install.yml -u root -k
}

function setBootLoad {
    ansible-playbook -i tsinghua_host set_boot_load.yml -u root -k
}

function idgMake {
    cd idg
    make clean
    ./load_host    
    cd ..
    ansible-playbook -i tsinghua_host idg_make.yml -u root -k
}

function idgInstall {
    ansible-playbook -i tsinghua_host idg_install.yml -u root -k
}

function idgUninstall {
    ansible-playbook -i tsinghua_host idg_uninstall.yml -u root -k
}

function idgRun {
    ansible-playbook -i tsinghua_host idg_run.yml -u root -k
}

function idgRunClient {
    ansible-playbook -i tsinghua_host idg_run_client.yml -u root -k
}

function idgStop {
    ansible-playbook -i tsinghua_host idg_stop.yml -u root -k
}

function idgAll {
    cd idg
    make clean
    ./load_host    
    cd ..    
    echo "==== Please input compile server's password ===="
    ansible-playbook -i tsinghua_host idg_make.yml -u root -k
    echo "==== Please input idg test nodes's password ===="    
    ansible-playbook -i tsinghua_host idg_install_run.yml -u root -k
}

function agentInstall {
    ansible-playbook -i tsinghua_host agent_install.yml -u root -k
}

function agentUninstall {
    ansible-playbook -i tsinghua_host agent_uninstall.yml -u root -k
}

function agentLoad {
    ansible-playbook -i tsinghua_host agent_load.yml -u root -k
}

function agentUnload {
    ansible-playbook -i tsinghua_host agent_unload.yml -u root -k
}

function agentRun {
    ansible-playbook -i tsinghua_host agent_run.yml -u root -k
}

function agentCheckrun {
    ansible-playbook -i tsinghua_host agent_checkrun.yml -u root -k
}

function agentStop {
    ansible-playbook -i tsinghua_host agent_stop.yml -u root -k
}

function agentAll {
    ansible-playbook -i tsinghua_host agent_install_load_run.yml -u root -k
}

function agentLoadrun {
    ansible-playbook -i tsinghua_host agent_loadrun.yml -u root -k
}

function initHost {
    ansible-playbook -i tsinghua_host init_host.yml -u root -k
}



# function installServer {
#     server="root@"${1-"10.1.10.10"}
# #    ssh $server "mkdir -p /opt/gcc/"
# #    scp /opt/gcc/glib.tar.xz $server:/opt/gcc/
# #    scp libc.conf $server:/etc/ld.so.conf.d/
# #    scp profile.gcc $server:/etc/profile.d/
# #    ssh $server "cd /opt/gcc/; tar xvf glib.tar.xz"
# #    ssh $server "mkdir -p $ROOT/"
# #    scp *.pem $server:$ROOT/
#     scp server $server:$ROOT/
# }
# 
# function installTest {
#     scp -P $port test  $logon:$ROOT/
    # scp -P $port testVlan $logon:$ROOT/
#     scp -P $port vlanClient $logon:$ROOT/
#     scp -P $port vlanServer $logon:$ROOT/
#     scp -P $port testStat $logon:$ROOT/
#     scp -P $port statClient $logon:$ROOT/
#     scp -P $port statServer $logon:$ROOT/
# }
# 
# function queryClient {
#     ssh -p $port $logon ps -A | grep client
# }
# 
# function stopClient {
#     ssh -p $port $logon "killall client"
# }
# 
# function pingClient {
#     ssh $logon ping $address -w1 -c1 | grep bytes | grep time
# }
# 
# function runNTP {
# #    ssh -p $port $logon "service ntpd stop"
# #    ssh -p $port $logon "ntpdate -d 10.1.10.10"
#     ssh -p $port $logon "ntpdate $host"
# }
# 
# 
# function runLog {
#     ssh -p $port $logon "rm $ROOT/client*.log"
#     ssh -p $port $logon "rm /var/log/*-*"
# }
# 
# function runClient {
# #    runNTP
#     #echo ssh -p $port $logon "$ROOT/run.sh &"  ' > /dev/null &'
#     ssh -p $port $logon "$ROOT/run.sh"  > /dev/null &
# }
# 
# function checkMachine {
#     pingClient
#     if [ $? -eq 0 ]; then
#         return;
#     fi
#     ssh $logon virsh destroy $machine
#     ssh $logon virsh create $machine.xml
# }
# 
# function checkRun {
#     queryClient
#     if [ $? -eq 0 ]; then
#         return;
#     fi
#     installLoad $face
#     runClient
# }
# 
# function runTemp {
# #    echo perform temp task
#     
#     ssh -p $port $logon "dmesg -c"
#     ssh -p $port $logon "reboot"
# 
   # ssh -p $port $logon "service iptables stop"
# 
# #    ssh -p $port $logon "ethtool -K $face sg off"
# #    ssh -p $port $logon "ethtool -K $face tso off"
# #    ssh -p $port $logon "ethtool -K $face gso off"
# #    ssh -p $port $logon "ethtool -K $face gro off"
# 
# #    scp -P $port ethtool-2.6.39-1.fc14.x86_64.rpm $logon:$ROOT/    
# #    ssh -p $port $logon "rpm -ivh $ROOT/ethtool-2.6.39-1.fc14.x86_64.rpm"
#     
# #    scp -P $port ether.sh $logon:$ROOT/
# #    ssh -p $port $logon "chmod +x $ROOT/ether.sh; $ROOT/ether.sh $face"
# #    ssh -p $port $logon "echo -e 'ntpdate 172.16.1.10\nsync\ncd $ROOT\ndmesg -c\n./client -n NODE172.16.1.${port:(-3)} -p3490\n' > $ROOT/run.sh; chmod +x $ROOT/run.sh"
# 
# #    ssh -p $port $logon "echo -e '0  *  *  *  * root ntpdate 172.16.1.10' >> /etc/crontab"
# #    ssh -p $port $logon "echo -e 'ethtool -K $face sg off' >> /etc/rc.local"
# #    ssh -p $port $logon "echo -e 'ethtool -K $face tso off' >> /etc/rc.local"
# #    ssh -p $port $logon "echo -e 'ethtool -K $face gso off' >> /etc/rc.local"
# #    ssh -p $port $logon "echo -e 'ethtool -K $face gro off' >> /etc/rc.local"
# 
# #    ssh -p $port $logon "chkconfig iptables off"
# #    ssh -p $port $logon "chkconfig ntpd off"
# #    ssh -p $port $logon "$ROOT/ether.sh $face"
# #    ssh -p $port $logon "rm /var/log/*-*"
# #    ssh -p $port $logon "rm /var/log/messages"
#     
# #    ssh -p $port $logon "echo -e '\nntpdate 172.16.1.10' >> /etc/rc.local"
# }

function processTask {
    case "$1" in
	ping)
		doPing
		;;
	cmd)
		doCommand
		;;
	ntp)
		doNtp
		;;
	load_nic)
		loadNIC
		;;        
    cross_ping)
        crossPing
		;;                
    proxy_ping)
        proxyPing
        ;;                        
	idg_make)
		idgMake
		;;
	idg_install)
		idgInstall
		;;
	idg_uninstall)
		idgUninstall
		;;
	idg_run)
		idgRun
		;;
	idg_runclient)
		idgRunClient
		;;        
	idg_stop)
		idgStop
		;;
	idg_all)
		idgAll
		;;
	agent_install)
        agentInstall
		;;
	agent_uninstall)
        agentUninstall
		;;
	agent_load)
		agentLoad
		;;
	agent_unload)
		agentUnload
    		;;        
	agent_run)
		agentRun
		;;
	agent_checkrun)
		agentCheckrun
		;;        
	agent_stop)
		agentStop
		;;
	agent_all)
		agentAll
		;;
	agent_loadrun)
		agentLoadrun
		;;        
	mlx_install)
		mlxInstall
		;;                
	arp_install)
		arpInstall
		;;                        
    set_boot_load)
        setBootLoad
    	;;                                
    init_host)
        initHost
    	;;                                        
	*)
		;;
	esac
}

if [ -z "$1" ]; then
	echo INVALID Command
	echo use $0 "[General cmds|IDG cmds|Agent cmds] [nodes]"
    echo "General commands:"    
    echo "  ping [nodes]       Test nodes accessibility"
    echo "  cmd  [nodes]       Execute shell command on nodes"
    echo "  ntp  [server]      Synchronize nodes time to server"
    echo "  load_nic [nodes]   Reload network interface card driver module"    
    echo "  cross_ping         Cross connectivity validate"        
    echo "  proxy_ping         connectivity validate through a proxy"            
    echo "  mlx_install        Install mlx4 module and set NIC"            
    echo "  arp_install        Install static arp rules"                
    echo "  set_boot_load      Set loading params on boot"                    
    echo "  init_host          Init a entire new host"                        
    
    echo
    
    echo "IDG test commands:"
    echo "  idg_make           Compile idg test codes"
    echo "  idg_install        Install idg test programs to nodes"
    echo "  idg_uninstall      Uninstall idg test programs from nodes"    
    echo "  idg_run            Run idg test programs on nodes"
    echo "  idg_runclient      Run idg client programs on nodes"    
    echo "  idg_stop           Stop idg test programs on nodes"
    echo "  idg_all            install and run idg test programs on nodes"
    
    echo
    
    echo "Sodero agent commands:"        
    echo "  agent_install      Install agent modules to nodes"
    echo "  agent_uninstall    Uninstall agent modules from nodes"    
    echo "  agent_load         Reload agent kernel module on nodes"
    echo "  agent_unload       Unload agent kernel module on nodes"    
    echo "  agent_run          Run agent programs on nodes"
    echo "  agent_checkrun     Run agent programs on nodes if not running"
    echo "  agent_stop         Stop agent programs on nodes"
    echo "  agent_all          Install and load and run agent programs on nodes"    
    echo "  agent_loadrun      Reload and run agent programs on nodes"    

	echo

else
	processTask ${1-"sodero"} $2
fi

