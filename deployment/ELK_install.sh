#!/bin/bash
# Install the relevant dependencies around the ELK stack and add relevant repositories 

if [ $(id -u) != "0" ]; then
echo "You must be the superuser (sudo) to run this script" >&2
exit 1
fi

## Install Elastic search
#Install Java
apt-get -y install openjdk-8-jdk

#Add Elastic Stack Repository
wget -qO - https://artifacts.elastic.co/GPG-KEY-elasticsearch | sudo apt-key add -

#Install Main HTTPS apt requests
apt-get -y install apt-transport-https ca-certificates wget

echo "deb https://artifacts.elastic.co/packages/7.x/apt stable main" | sudo tee /etc/apt/sources.list.d/elastic-7.x.list



#Install Elastic Search
apt-get update 
apt-get -y install elasticsearch

#Enable It On Boot
systemctl start elasticsearch.service
systemctl enable elasticsearch.service

# Test to verify it is working
curl localhost:9200


## General Dependancies
apt-get update
apt-get -y install curl

# Install the build tools (dpkg-dev g++ gcc libc6-dev make, not required but recomended)
apt-get -y install build-essential


#Install Logstash
apt-get -y install logstash
cp logstash_AI.conf /etc/logstash/conf.d
systemctl start logstash
systemctl enable logstash



# Installing Kibana
apt-get -y install kibana
systemctl start kibana
systemctl enable kibana


# Test to verify it is working
curl -i localhost:5601


echo "Install Complete, refer to Localhost:5601 for the kibana dashboards"

