#! /bin/bash

### BEGIN INIT INFO
# Provides:          FROST demo
# Required-Start:
# Required-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start & stops FROST demo docker container
# Description:       Start & stops FROST demo docker container
### END INIT INFO

start() {
    docker-compose -f $DIR/docker-compose.yaml up
}

stop() {
    docker-compose -f $DIR/docker-compose.yaml down
}

case "$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  restart)
    stop
    start
    ;;
  status)
    docker-compose -f $DIR/docker-compose.yaml ps
    ;;
    *)
       echo "Usage: $0 {start|stop|status|restart}"
esac

exit 0