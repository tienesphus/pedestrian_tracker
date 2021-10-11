# Deployment Information
The following is a collection of files that can be used to both install the relevant applications to that two systems and the relevant configuration files to send the data from filebeat to Logstash successfully. Each configuration file has the location that the file is required to be placed for reference. The applications must be install first however. This can be done through the provided scripts "AI_Install.sh" and "ELK_Install.sh".  

The "dashboards.ndjson" is to be used in conjunction with Kibana. This will import the relevant indices into the application to display the dashboard. This can be achieved by accessing the "saved objects" menu within the Kibana menu. From here the user can click import to load this file which will then load all of the preconfigured dashboards

