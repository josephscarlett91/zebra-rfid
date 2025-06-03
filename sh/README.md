
# GET 
to call http api, refer to doc in ip url admin menu
example execution 
$ ./zrfid GET network 

## NOTE 
install jq to pretty print output of json
$ ./zrfid GET network | jq

# Connect to Hotspot
get ip address
$ ip a 

configure endpoint to ip location 

# Start APP 
run command in cli to begin app or stop
modify filter to alter a prefix to read

$ ./zrfid PUT mode Config_AdjFilter.json
$ ./zrfid POST "" start.json 
$ ./zrfid POST "" stop.json 


