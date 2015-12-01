DOCKER_IMAGE=ncrmnt/mb7707-devel
.docker-pull:
	docker pull $(DOCKER_IMAGE)
	touch $(@)

.docker-container-id: .docker-pull
	[ -d bin ] || mkdir ./bin
	docker run -d \
	-v `pwd`:/opt/project/src/ \
	-w /opt/project/src \
	--entrypoint /bin/bash -t $(DOCKER_IMAGE) > $(@)

docker-compile: .docker-container-id
	docker start `cat $(<)`
	docker exec `cat $(<)` /bin/bash --login -c make

docker-package: .docker-container-id
	docker start `cat $(<)`
	docker exec `cat $(<)` /bin/bash --login -c dpkg-buildpackage -us -uc
#Old docker (debian jessie) doesn't support wildcards in cp, so do it dirty
	docker exec `cat $(<)` find .. -type f -maxdepth 1|while read line; do \
		docker cp `cat $(<)`:/opt/project/`basename $$line` ./bin/; \
	done

docker-shell: .docker-container-id
	docker exec -it `cat $(<)` /bin/bash

docker-commit: .docker-container-id
	docker commit `cat $(<)` > .docker_image_id
	docker ps|grep 7707-devel | awk '{print $$1}'| while read line; do \
		sudo docker kill $$line; \
	done
	docker rmi -f $(DOCKER_IMAGE)
	docker tag `cat .docker_image_id` $(DOCKER_IMAGE)

docker-install:
	docker exec `cat .docker-container-id` dpkg -i ./bin/*.deb
