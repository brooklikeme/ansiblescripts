#server
echo "starting proteusys server..."
nohup python /home/proteusys/proteusys_server.py > /home/proteusys/proteusys.log 2>&1 &

#edfa
echo "initializing edfa"
proteusys_edfa -a0 -c22 -l2 0 3 >> /home/proteusys/edfa.log
proteusys_edfa -a0 -c22 -l2 0 3 >> /home/proteusys/edfa.log

#wss
echo "initializing wss"
proteusys_wss -c8 -l96 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2 1 3 5 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 0 0 0 0 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 >> /home/proteusys/wss.log 
