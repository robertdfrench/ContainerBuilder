# Server Control

Prerequisite: On Titan
* module load python
* pip install --user python-openstackclient

Servers may be spun up anywhere on the ORNL network and also the Titan login nodes. To obtain the credentials required for access take the following steps.
* Login to `cloud.cades.ornl.gov`
* Navigate to `Compute -> Access & Security -> API Access`
* Click `Download OpenStack RC File v3`
* Rename downloaded file as my-openrc.sh and move it to `ContainerBuilder/Scripts`
* Modify `openrc.sh` to hardcode `$OS_PASSWORD_INPUT` and remove the interactive prompt

To initiate the Containerbuilder service several steps are required
* Bring up the BuilderQueue OpenStack instance
* Create the Builder master OpenStack image which will be used by the Builder Queue

To bring up a new builder instance:
```
ContainerBuilder/Scripts/BringUpQueue
```
After bringup an SSH key will be created: `ContainerBuilderKey`

 `ContainerBuilderKey` provides SSH access to the BuilderQueue as well as all of the builders

During bring up `openrc.sh` will be copied to `/home/queue` on the `BuilderQueue` host, these credentials are required to bring up builders.

After the Queue has been brought up the master builder image must be created:
```
ContainerBuilder/Scripts/CreateBuilderImage
```

`ContainerBuilder` should now be functional


To login to the queue or a builder:
```
ssh -i ./ContainerBuilder cades@<BuilderQueueIP/BuilderIP>
```

To destroy a new queue instance:
```
ContainerBuilder/Scripts/TearDownQueue
```

---
Note: `openstack list` can be called to show all active OpenStack instances including their ID, name, and IP

Note: To check if the BuilderQueue and Builder services are running ssh to each node and run `systemctl status`. To diagnose issues use `sudo journalctl`
