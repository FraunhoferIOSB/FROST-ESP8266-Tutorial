#!/bin/bash
#################################
### init script ###
#################################
AUTO_LOGIN=0
CREATE_USER=0
SECURE_USER=0
SET_KEYBOARD_LAYOUT=0
WORK_DIR="./FROST-ESP8266-Tutorial/RaspberryPi"
GIT_URL="https://github.com/FraunhoferIOSB/FROST-ESP8266-Tutorial.git"
DEMO_USER="demo"
DEMO_USER_PWD="frostdemo"
WIFI_SSID="frost-network"
WIFI_PWD="frostWEB"
PI_USER_PWD="sweetPIE"
KEYBOARD_LAYOUT="de"
INTERFACE_COMMENT"#DEMO_INTERFACE"

CURRENT_DIR=`dirname $(realpath -s $0)`
SCRIPT_FILE=`basename "$0"`

### parse CLI arguments

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    --create-user)
    CREATE_USER=1
    shift
    ;;
	-s|--secure)
    SECURE_USER=1
    shift
    ;;
	--auto-login)
    AUTO_LOGIN=1
    shift
    ;;
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
	--new-pwd)
    PI_USER_PWD="$2"
    shift
    shift
    ;;
	--keyboard-layout)
    SET_KEYBOARD_LAYOUT=1
	KEYBOARD_LAYOUT="$2"
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


if [ $SET_KEYBOARD_LAYOUT -eq 1 ]
then
	echo 'setting keyboard layout to $KEYBOARD_LAYOUT'
	sudo sed -i "s/^#XKBLAYOUT=.*/XKBLAYOUT=\"$KEYBOARD_LAYOUT\"/" /etc/default/keyboard
fi

if [ $SECURE_USER -eq 1 ]
then
	echo "changing password of user pi to $PI_USER_PWD"
	echo "pi:$PI_USER_PWD" | sudo chpasswd
fi

if [ $CREATE_USER -eq 1 ]
then
	if [ -z "$(getent passwd $DEMO_USER)" ]
	then
		echo "creating user: $DEMO_USER with password $DEMO_USER_PWD"
		sudo adduser --quiet --disabled-password --gecos "demo user" --ingroup sudo $DEMO_USER
	fi
	echo "updating password for user: $DEMO_USER to $DEMO_USER_PWD"
	echo "$DEMO_USER:$DEMO_USER_PWD" | sudo chpasswd
else
	DEMO_USER=$USER
fi

if [ $AUTO_LOGIN -eq 1 ]
then
	echo "set auto-login to demo user"
	mkdir -pv /etc/systemd/system/getty@tty1.service.d
	cat >/etc/systemd/system/getty@tty1.service.d/autologin.conf <<EOL
[Service]
ExecStart=-/sbin/agetty --autologin $DEMO_USER --noclear I 38400 linux
EOL
fi

echo "installing docker ..."
sudo curl -sSL https://get.docker.com | sh
sudo usermod -aG docker $USER
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

sudo chown -R $DEMO_USER $GIT_LOCAL

echo "setting up wifi hotspot ..."
sudo apt-get update && apt-get -y install hostapd dnsmasq
sudo systemctl stop dnsmasq
sudo systemctl stop hostapd
# disabled dhcp for WiFi
echo "denyinterfaces wlan0" | sudo tee -a /etc/dhcpcd.conf > /dev/null
# configure static IP for WiFi
if ! grep -q "#DEMO_INTERFACE" /etc/network/interfaces; then
  sudo cat $GIT_LOCAL/RaspberryPi/new_interface >> /etc/network/interfaces
fi
# deploy hostapd config
sudo cp -rf $GIT_LOCAL/RaspberryPi/hostapd.conf /etc/hostapd/hostapd.conf
sudo sed -i "s/^ssid=.*/ssid=$WIFI_SSID/" /etc/hostapd/hostapd.conf
sudo sed -i "s/^wpa_passphrase=.*/wpa_passphrase=$WIFI_PWD/" /etc/hostapd/hostapd.conf
echo "DAEMON_CONF=\"/etc/hostapd/hostapd.conf\"" | sudo tee -a /etc/default/hostapd > /dev/null
# configure dnsmasq
sudo cp -rf $GIT_LOCAL/RaspberryPi/dnsmasq.conf /etc/dnsmasq.conf
sudo systemctl start dnsmasq
sudo systemctl unmask hostapd
sudo systemctl enable hostapd
sudo systemctl start hostapd

sudo reboot
