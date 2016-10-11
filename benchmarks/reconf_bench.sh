define(){ IFS='\n' read -r -d '' ${1} || true; }
declare -A pids
declare -A rounds
redirection=( "> out" "2> err" "< /dev/null" )

define HELP <<'EOF'
Script for starting DARE
usage  : $0 [options]
options: --app                # app to run
EOF

usage () {
    echo -e "$HELP"
}

ErrorAndExit () {
  echo "ERROR: $1"
  exit 1
}

Sync() {
    sed -i "s/group_size = .*/group_size = $1;/" ${DAREDIR}/target/nodes.local.cfg
    for ((i=0; i<$1; ++i));
    do
        cmd=( "scp" "${DAREDIR}/target/nodes.local.cfg" "$USER@${servers[$i]}:${DAREDIR}/target/" )
        $("${cmd[@]}")
        echo "Sync COMMAND: "${cmd[@]}
    done
}

ForceAbsolutePath () {
  case "$2" in
    /* )
      ;;
    *)
      ErrorAndExit "Expected an absolute path for $1"
      ;;
  esac
}

StartDare() {
    for ((i=0; i<${group_size}; ++i)); do
        srv=${servers[$i]}
        config_dare=( "server_type=start" "server_idx=$i" "config_path=${DAREDIR}/target/nodes.local.cfg" "dare_log_file=$PWD/srv${i}_1.log" "mgid=$DGID" "LD_PRELOAD=${DAREDIR}/target/interpose.so" "$BIN")
        cmd=( "ssh" "$USER@${servers[$i]}" "${config_dare[@]}" "nohup" "${run_dare}" "${redirection[@]}" "&" "echo \$!" )
        pids[$srv]=$("${cmd[@]}")
        rounds[$srv]=2
        #echo "COMMAND: "${cmd[@]}
        echo -e "\tp$i ($srv) -- pid=${pids[$srv]}"
    done
    #echo -e "\n\tinitial servers: ${!servers[]}${!pids[@]}"
    #echo -e "\t...and their PIDs: ${pids[@]}"
}

StopDare() {
    for srv in "${!pids[@]}"; do
        cmd=( "ssh" "$USER@$srv" "kill -s SIGINT" "${pids[$srv]}" )
        echo "Executing: ${cmd[@]}"
        $("${cmd[@]}")
    done
}

FindLeader() {
    leader=""
    max_idx=-1
    max_term=""
 
    for ((i=0; i<${group_size}; ++i)); do
        srv=${servers[$i]}
        # look for the latest [T<term>] LEADER 
        cmd=( "ssh" "$USER@$srv" "grep -r \"] LEADER\"" "$PWD/srv${i}_$((rounds[$srv]-1)).log" )
        #echo ${cmd[@]}
        grep_out=$("${cmd[@]}")
        if [[ -z $grep_out ]]; then
            continue
        fi
        terms=($(echo $grep_out | awk '{print $2}'))
        for j in "${terms[@]}"; do
           term=`echo $j | awk -F'T' '{print $2}' | awk -F']' '{print $1}'`
           if [[ $term -gt $max_term ]]; then 
                max_term=$term
                leader=$srv
                leader_idx=$i
           fi
        done
    done
    #echo "Leader: p${leader_idx} ($leader)"
}

RemoveLeader() {
    FindLeader
    if [[ -z $leader ]]; then
        echo -e "\n\tNo leader [$leader]"
        return 1
    fi
    #echo ${!pids[@]}
    #echo ${pids[@]}
    if [[ -z ${pids[$leader]} ]]; then
        echo -e "\n\tNo PID for the leader $leader"
        return 1
    fi
    cmd=( "ssh" "$USER@$leader" "kill -s SIGINT" "${pids[$leader]}" )
    $("${cmd[@]}")
    unset pids[$leader]
    echo -e "\tremoved p${leader_idx} ($leader)"
    #echo -e "\n\tservers after removing the leader p${leader_idx} ($leader): ${!pids[@]}"
    #echo -e "\t...and their PIDs: ${pids[@]}"
    #echo ${cmd[@]}
    maj=$(bc -l <<< "${group_size}/2.")
    if [[ ${#pids[@]} < $maj ]]; then
        ErrorAndExit "...not enough servers!"
    fi
    return 0
}

# Stop a server that is not the leader
RemoveServer() {
    FindLeader
    for ((i=0; i<${group_size}; ++i)); do
        srv=${servers[$i]}
        if [[ "x$srv" == "x$leader" ]]; then
            continue
        fi
        if [[ "x${pids[$srv]}" == "x" ]]; then
            continue
        fi
        cmd=( "ssh" "$USER@$srv" "kill -s SIGINT" "${pids[$srv]}" )
        #echo -e "\tcmd: ${cmd[@]}"
        $("${cmd[@]}")
        unset pids[$srv]
        echo -e "\tremoved p$i ($srv) -- p$leader_idx is the leader"
        #echo -e "\tservers after removing p$i ($srv): ${!pids[@]}"
        #echo -e "\t...and their PIDs: ${pids[@]}"
        #echo ${cmd[@]}
        break
    done
    maj=$(bc -l <<< "${group_size}/2.")
    if [[ ${#pids[@]} < $maj ]]; then
        ErrorAndExit "...not enough servers!"
    fi
}

DAREDIR=$PWD/..
APP=""
for arg in "$@"
do
    case ${arg} in
    --help|-help|-h)
        usage
        exit 1
        ;;
    --op=*)
        OPCODE=`echo $arg | sed -e 's/--op=//'`
        OPCODE=`eval echo ${OPCODE}`    # tilde and variable expansion
        ;;
    --app=*)
        APP=`echo $arg | sed -e 's/--app=//'`
        APP=`eval echo ${APP}`    # tilde and variable expansion
        ;;
    esac
done

if [[ "x$APP" == "x" ]]; then
    ErrorAndExit "No app defined: --app"
elif [[ "$APP" == "ssdb" ]]; then
    run_dare="${DAREDIR}/apps/ssdb/ssdb-master/ssdb-server ${DAREDIR}/apps/ssdb/ssdb-master/ssdb.conf"
elif [[ "$APP" == "redis" ]]; then
    run_dare="${DAREDIR}/apps/redis/install/bin/redis-server"
fi

# list of allocated nodes, e.g., nodes=(n112002 n112001 n111902)
nodes=(10.22.1.1 10.22.1.2 10.22.1.3 10.22.1.4 10.22.1.5 10.22.1.6 10.22.1.7 10.22.1.8 10.22.1.9)
node_count=${#nodes[@]}
if [ $node_count -lt 8 ]; then
    ErrorAndExit "At least 8 nodes are required."
fi
echo "Allocated ${node_count} nodes:" > nodes
for ((i=0; i<${node_count}; ++i)); do
    echo "$i:${nodes[$i]}" >> nodes
done
group_size=9

for ((i=1; i<$node_count; ++i)); do
    servers[$((i-1))]=${nodes[$i]}
done
echo ">>> $(($node_count-1)) servers: ${servers[@]}"

rm -f *.log

########################################################################

Stop() {
    sleep 0.2
    StopDare
    exit 1
}

Start() {
    Sync $group_size
    echo -e "Starting $group_size servers..."
    StartDare
    echo "done"

    sleep 2

    sleep 0.5
    
    if [[ "x$1" == "xstop" ]]; then
        Stop
    fi    
}

FailLeader() {
    echo -e "Removing the leader..."
    while true; do
        RemoveLeader
        ret=$?
        #echo "ret=$ret"
        if [ $ret -eq 0 ]; then 
            break;    
        fi
        sleep 0.05
    done
    echo "done"

    sleep 1
    
    if [[ "x$1" == "xstop" ]]; then
        Stop
    fi  
}

FailServer() {
    echo -e "Removing a server (non-leader)..."
    RemoveServer
    echo "done"

    sleep 0.7
    
    if [[ "x$1" == "xstop" ]]; then
        Stop
    fi
}

########################################################################

Start

FailLeader

FailServer 

FailLeader stop
