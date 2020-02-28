#!/bin/bash
#################################
### init script ###
#################################
WORK_DIR="./FROST-ESP8266-Tutorial/RaspberryPi"
GIT_URL="https://github.com/FraunhoferIOSB/FROST-ESP8266-Tutorial.git"
DEMO_USER="demo"
DEMO_USER_PWD="frostdemo"
WIFI_SSID="frost-network"
WIFI_PWD="frostWEB"

if ! [ $(id -u) = 0 ]; then
   echo "This script must be run a root!"
   exit 1
fi

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -u|--user)
    DEMO_USER="$2"
    shift
    shift
    ;;
	-p|--password)
    DEMO_USER_PWD="$2"
    shift
    shift
    ;;
	--wifi-ssid)
    WIFI_SSID="$2"
    shift
    shift
    ;;
	--wifi-pwd)
    WIFI_PWD="$2"
    shift
    shift
    ;;
    --default)
    DEFAULT=YES
    shift
    ;;
    *)    # unknown option
    POSITIONAL+=("$1")
    shift
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

if [ -z "$(getent passwd $DEMO_USER)" ]
then
	echo "creating user: $DEMO_USER with password $DEMO_USER_PWD"
	sudo adduser --quiet --disabled-password --gecos "demo user" --ingroup sudo $DEMO_USER
fi
echo "$DEMO_USER:$DEMO_USER_PWD" | sudo chpasswd

echo "installing docker ..."
sudo curl -sSL https://get.docker.com | sh
sudo usermod -aG docker $DEMO_USER
sudo apt-get install -y python python-pip libffi-dev libssl-dev
sudo apt-get remove -y python-configparser
sudo pip install docker-compose
# auto-start docker
sudo systemctl enable docker
sudo apt-get install git

echo "cloning git reporitory  ..."
GIT_LOCAL="$(eval echo "~$DEMO_USER")/$(basename $GIT_URL | cut -f 1 -d '.')"
if [ ! -d $GIT_LOCAL/.git ]
then
    git clone "$GIT_URL" $GIT_LOCAL
else
    cd $GIT_LOCAL
    git pull "$GIT_URL"
fi
WORK_DIR=$GIT_LOCAL/RaspberryPi

sudo chown -R $DEMO_USER $GIT_LOCAL

docker-compose -f $WORK_DIR/docker-compose.yaml pull
sudo cp -rf $WORK_DIR/frost-demo.sh /etc/init.d/frost-demo
sudo sed -i "s/\$DIR/$(echo "$WORK_DIR" | sed "s;/;\\\/;g")/" /etc/init.d/frost-demo
sudo update-rc.d -f frost-demo remove
sudo update-rc.d frost-demo defaults

echo "setting up wifi hotspot ..."
sudo apt-get update && apt-get -y install hostapd dnsmasq
sudo systemctl stop dnsmasq
sudo systemctl stop hostapd
# disabled dhcp for WiFi
echo "denyinterfaces wlan0" | sudo tee -a /etc/dhcpcd.conf > /dev/null
# configure static IP for WiFi
if ! grep -q "#DEMO_INTERFACE" /etc/network/interfaces; then
  sudo cat $WORK_DIR/new_interface >> /etc/network/interfaces
fi
# deploy hostapd config
sudo cp -rf $WORK_DIR/hostapd.conf /etc/hostapd/hostapd.conf
sudo sed -i "s/^ssid=.*/ssid=$WIFI_SSID/" /etc/hostapd/hostapd.conf
sudo sed -i "s/^wpa_passphrase=.*/wpa_passphrase=$WIFI_PWD/" /etc/hostapd/hostapd.conf
echo "DAEMON_CONF=\"/etc/hostapd/hostapd.conf\"" | sudo tee -a /etc/default/hostapd > /dev/null
# configure dnsmasq
sudo cp -rf $WORK_DIR/dnsmasq.conf /etc/dnsmasq.conf
sudo systemctl start dnsmasq
sudo systemctl unmask hostapd
sudo systemctl enable hostapd
sudo systemctl start hostapd

sudo reboot
