/****************************************************************************
 *
 *   Copyright (c) 2012-2019 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file px4_simple_app.c
 * Minimal application example for PX4 autopilot
 *
 * @author Example User <mail@example.com>
 */

__EXPORT int px4_simple_app_main(int argc, char *argv[]);

void * produce(void *arg){
	char counter = 0;

	struct mq_attr attr;

	attr.mq_flags = 0;
	attr.mq_maxmsg = CAPACITY;
	// вес одного сообщения, в нашем случае одно целое число
	attr.mq_msgsize = sizeof(char);
	attr.mq_curmsgs = 0;

	mqd_t Queue = mq_open(QUEUE_NAME, O_WRONLY | O_CREAT, QUEUE_PERMISSIONS, &attr );
        PX4_INFO("Accelerometer:\t%8.4f\t%8.4f\t%8.4f",
                    (double)raw.accelerometer_m_s2[0],
                    (double)raw.accelerometer_m_s2[1],
                    (double)raw.accelerometer_m_s2[2]);

		sleep(1); // заснуть на секунду
		// https://nuttx.apache.org/docs/latest/reference/user/04_message_queue.html#c.mq_send
		mq_send(Queue, &counter, sizeof(char), 2);
		
	is_done = 1;

	// закрыть очередь
	// https://nuttx.apache.org/docs/latest/reference/user/04_message_queue.html#c.mq_close

	mq_close(Queue);

	return NULL;


}

void * consume(void *arg){

	char digit;
	unsigned int prior;

	struct mq_attr attr;

	attr.mq_flags = 0;
	attr.mq_maxmsg = CAPACITY;
	// вес одного сообщения, в нашем случае одно целое число
	attr.mq_msgsize = sizeof(char);
	attr.mq_curmsgs = 0;

	mqd_t Queue = mq_open(QUEUE_NAME, O_RDONLY | O_CREAT , QUEUE_PERMISSIONS, &attr );

	//mqd_t Queue = mq_open(QUEUE_NAME, O_RDONLY);

	while(!is_done ){
		mq_receive(Queue, &digit, sizeof(char), &prior);
		PX4_INFO("Receiving %d with priority %d\n", digit, prior);
		sleep(1);
	}

	// получаем все оставшиеся сообщения
	// https://nuttx.apache.org/docs/latest/reference/user/structures.html#c.timespec
	struct timespec time_to_wait;
	time_to_wait.tv_sec = 1;
	time_to_wait.tv_nsec = 0;

	// здесь используем функцию получения сообщения с ожиданием 2 секунды, чтобы выйти в случае пустой очереди
	while(mq_timedreceive(Queue, &digit, sizeof(char), &prior, &time_to_wait) != EAGAIN){
		PX4_INFO("Receiving %d with priority %d\n", digit, prior);
		sleep(1);
	}


	// закрыть очередь
	// https://nuttx.apache.org/docs/latest/reference/user/04_message_queue.html#c.mq_close
	mq_close(Queue);

	return NULL;

}

int px4_simple_app_main(int argc, char *argv[])
{

	int sensor_sub_fd = orb_subscribe(ORB_ID(vehicle_acceleration));
	/* limit the update rate to 5 Hz */
	orb_set_interval(sensor_sub_fd, 100);

	/* advertise attitude topic */
	struct vehicle_attitude_s att;
	memset(&att, 0, sizeof(att));
	orb_advert_t att_pub = orb_advertise(ORB_ID(vehicle_attitude), &att);

	/* one could wait for multiple topics with this technique, just using one here */
	px4_pollfd_struct_t fds[] = {
		{ .fd = sensor_sub_fd,   .events = POLLIN },
		/* there could be more file descriptors here, in the form like:
		 * { .fd = other_sub_fd,   .events = POLLIN },
		 */
	};

	pthread_t consumer_thread;
	pthread_t producer_thread;

	// создать и запустить поток  производителя
	pthread_create(&producer_thread, NULL, produce, NULL);
	// создать и запустить поток потребителя
	pthread_create(&consumer_thread, NULL, consume, NULL);

	// ждать завершения
	pthread_join(producer_thread, NULL);
	pthread_join(consumer_thread, NULL);






	PX4_INFO("exiting app");

	return 0;
}

