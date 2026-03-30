#!/bin/bash
# Check SSH service status
sudo systemctl status ssh --no-pager

# Check SSH listening address
sudo ss -tlnp | grep ssh

# Check network interfaces
ip a