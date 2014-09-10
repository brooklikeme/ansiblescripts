case "$(pidof agent_client | wc -w)" in
0)  echo "agent_client is not running, run it..."
    ./run.sh
    ;;
1)  echo "agent_client is running"
    ;;
esac