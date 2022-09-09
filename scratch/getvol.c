#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

int main(int argc, char **argv) {

	long min, max, vol;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "Master";

	for (;;sleep(1)) {
		snd_mixer_open(&handle, 0);
		snd_mixer_attach(handle, card);
		snd_mixer_selem_register(handle, NULL, NULL);
		snd_mixer_load(handle);


		snd_mixer_selem_id_alloca(&sid);
		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, selem_name);
		snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
		snd_mixer_selem_get_playback_volume(elem, 1, &vol);
		printf("%ld\n", vol * 100 / max);
	}

	snd_mixer_close(handle);
}
