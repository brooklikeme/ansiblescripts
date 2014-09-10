case "$(cat /etc/rc.d/rc.local | grep mlx4_en | wc -l)" in
0)  echo "mlx param has not been set in rc.local, set it..."
    echo -e "\n# Load 10g NIC driver\nmodprobe -r mlx4_en; modprobe mlx4_en" >> /etc/rc.d/rc.local
    ;;
1)  echo "mlx param has been set"
    ;;
esac
    
case "$(cat /etc/rc.d/rc.local | grep arp_install | wc -l)" in
0)  echo "arp_install param has not been set in rc.local, set it..."
    echo -e "\n# set static arp rules\n. /root/sodero/arp_install.sh" >> /etc/rc.d/rc.local
    ;;
1)  echo "arp_install param has been set"
    ;;    
esac