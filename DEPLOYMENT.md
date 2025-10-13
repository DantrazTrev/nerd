# NERD Production Deployment Guide

## Quick Start

```bash
# Clone and install
git clone https://github.com/yourusername/nerd.git
cd nerd
sudo ./install.sh

# Start the service
sudo systemctl start nerd
sudo systemctl enable nerd

# Check status
sudo systemctl status nerd
```

## System Requirements

### Minimum Requirements
- Linux kernel 4.19+ with raw socket support
- 2 GB RAM
- 1 GB disk space
- Root privileges for network operations

### Recommended Requirements
- Linux kernel 5.4+
- 4 GB RAM
- 10 GB disk space for flow persistence
- Dedicated network interface for flow traffic

### Network Requirements
- Raw socket support (CAP_NET_RAW capability)
- Network admin privileges (CAP_NET_ADMIN)
- Open port 31337 (configurable) for flow communication

## Installation Steps

### 1. Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git
```

**RHEL/CentOS/Fedora:**
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake git
```

### 2. Build and Install

```bash
# Clone repository
git clone https://github.com/yourusername/nerd.git
cd nerd

# Run installation script
sudo ./install.sh
```

### 3. Configure NERD

Edit `/etc/nerd/nerd.conf`:
```ini
[network]
interface = eth0          # Your network interface
circulation_rate = 10     # Packets per second
port = 31337             # Communication port

[security]
require_auth = true      # Enable in production
auth_key = <your-secret-key>

[logging]
log_level = 1            # 1=INFO for production
log_file = /var/log/nerd.log
```

### 4. Start Service

```bash
# Start NERD service
sudo systemctl start nerd

# Enable on boot
sudo systemctl enable nerd

# Check status
sudo systemctl status nerd
```

## Configuration

### Network Interface Selection

Choose the appropriate network interface:
```bash
# List available interfaces
ip link show

# Test with specific interface
sudo nerd -i eth1 testflow
```

### Security Configuration

For production environments, enable authentication:
```ini
[security]
require_auth = true
auth_key = $(openssl rand -hex 32)
enable_tls = true
cert_file = /etc/nerd/cert.pem
key_file = /etc/nerd/key.pem
```

Generate TLS certificates:
```bash
openssl req -x509 -newkey rsa:4096 -keyout /etc/nerd/key.pem \
    -out /etc/nerd/cert.pem -days 365 -nodes
```

### Performance Tuning

Optimize for high-traffic environments:
```ini
[network]
circulation_rate = 50     # Increase for more throughput
buffer_size = 262144      # Larger buffer for high traffic
worker_threads = 8        # Match CPU cores

[storage]
max_cache_size = 500      # Increase cache for better performance
```

## Operations

### Starting and Stopping

```bash
# Start service
sudo systemctl start nerd

# Stop service
sudo systemctl stop nerd

# Restart service
sudo systemctl restart nerd

# Reload configuration
sudo systemctl reload nerd
```

### Monitoring

**Check service status:**
```bash
sudo systemctl status nerd
```

**View logs:**
```bash
# Live logs
sudo journalctl -u nerd -f

# Last 100 lines
sudo journalctl -u nerd -n 100

# Log file
sudo tail -f /var/log/nerd.log
```

**Monitor network activity:**
```bash
# Watch packet traffic
sudo tcpdump -i eth0 port 31337

# Monitor resource usage
htop -p $(pgrep nerd)
```

### Backup and Recovery

**Backup flow data:**
```bash
sudo tar -czf nerd-backup-$(date +%Y%m%d).tar.gz \
    /var/lib/nerd /etc/nerd
```

**Restore from backup:**
```bash
sudo systemctl stop nerd
sudo tar -xzf nerd-backup-20231201.tar.gz -C /
sudo systemctl start nerd
```

## Troubleshooting

### Common Issues

**1. Permission Denied**
```
Error: Failed to create raw socket: Permission denied
Solution: Run with sudo or as root
```

**2. Interface Not Found**
```
Error: Failed to get interface index
Solution: Check interface name with 'ip link show'
```

**3. Port Already in Use**
```
Error: Failed to bind socket: Address already in use
Solution: Change port in config or stop conflicting service
```

**4. High Memory Usage**
```
Solution: Reduce buffer_size and max_cache_size in config
```

### Debug Mode

Run in debug mode for detailed output:
```bash
sudo nerd -d -c /etc/nerd/nerd.conf
```

### Performance Issues

1. **Slow circulation:**
   - Increase `circulation_rate`
   - Reduce `max_packet_age`
   - Check network congestion

2. **Packet loss:**
   - Increase `buffer_size`
   - Add more worker threads
   - Check network interface speed

3. **High CPU usage:**
   - Reduce `circulation_rate`
   - Optimize number of active flows
   - Check for network loops

## Security Considerations

### Network Isolation

Isolate NERD traffic on a dedicated VLAN:
```bash
# Create VLAN interface
sudo ip link add link eth0 name eth0.100 type vlan id 100
sudo ip link set eth0.100 up
sudo ip addr add 192.168.100.1/24 dev eth0.100

# Configure NERD to use VLAN
# In /etc/nerd/nerd.conf:
interface = eth0.100
```

### Firewall Configuration

Configure firewall rules:
```bash
# Allow NERD traffic
sudo iptables -A INPUT -p tcp --dport 31337 -j ACCEPT
sudo iptables -A OUTPUT -p tcp --sport 31337 -j ACCEPT

# Restrict to specific IPs
sudo iptables -A INPUT -p tcp --dport 31337 -s 192.168.1.0/24 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 31337 -j DROP
```

### Access Control

Limit access to configuration and logs:
```bash
# Restrict config access
sudo chmod 600 /etc/nerd/nerd.conf
sudo chown root:root /etc/nerd/nerd.conf

# Secure log files
sudo chmod 640 /var/log/nerd.log
sudo chown root:adm /var/log/nerd.log
```

## Scaling and High Availability

### Multiple Instances

Run multiple NERD instances on different interfaces:
```bash
# Instance 1
sudo nerd -c /etc/nerd/nerd1.conf -i eth0

# Instance 2
sudo nerd -c /etc/nerd/nerd2.conf -i eth1
```

### Load Balancing

Use multiple interfaces for load distribution:
```ini
[network]
interface = bond0  # Bonded interface for redundancy
```

### Clustering

Deploy NERD across multiple nodes for high availability:
1. Configure same `auth_key` on all nodes
2. Use shared storage for persistence
3. Implement network-level failover

## Monitoring Integration

### Prometheus Metrics

Export metrics for monitoring (future feature):
```ini
[monitoring]
enable_metrics = true
metrics_port = 9090
```

### Log Aggregation

Forward logs to centralized logging:
```bash
# Configure rsyslog
echo "*.* @@logserver.example.com:514" >> /etc/rsyslog.d/nerd.conf
sudo systemctl restart rsyslog
```

## Updates and Maintenance

### Updating NERD

```bash
# Stop service
sudo systemctl stop nerd

# Backup current installation
sudo cp /usr/local/bin/nerd /usr/local/bin/nerd.bak

# Pull latest changes and rebuild
cd /path/to/nerd
git pull
sudo ./install.sh

# Restart service
sudo systemctl start nerd
```

### Log Rotation

Configure automatic log rotation:
```bash
# Create logrotate config
cat > /etc/logrotate.d/nerd << EOF
/var/log/nerd.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 root root
    postrotate
        systemctl reload nerd
    endscript
}
EOF
```

## Production Checklist

- [ ] Configure appropriate network interface
- [ ] Enable authentication with strong key
- [ ] Set up TLS encryption for secure communication
- [ ] Configure firewall rules
- [ ] Set up log rotation
- [ ] Configure monitoring and alerting
- [ ] Create backup strategy
- [ ] Document network topology
- [ ] Test failover procedures
- [ ] Review security settings

## Support

For production support and enterprise features:
- Documentation: https://github.com/yourusername/nerd/wiki
- Issues: https://github.com/yourusername/nerd/issues
- Community: https://discord.gg/nerd-community

## License

NERD is experimental software. Use in production at your own risk.
Ensure compliance with network policies and regulations.