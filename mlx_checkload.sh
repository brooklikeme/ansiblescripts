case "$(lsmod | grep mlx4_en | wc -l)" in
0)  echo "module has not been load, load it..."
    modprobe -r mlx4_en; modprobe mlx4_en
    ;;
2)  echo "module has been load"
    ;;
esac