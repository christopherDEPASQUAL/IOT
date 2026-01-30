<script>
	import { onMount } from 'svelte';

	// URL du bridge WebSocket
	const WS_URL = `ws://localhost:${import.meta.env.WS_PORT}`;

	// Topic MQTT qui publie la meteo
	const MQTT_TOPIC = import.meta.env.MQTT_TOPIC;

	// Topic MQTT pour les commandes
	const COMMAND_TOPIC = import.meta.env.MQTT_TOPIC_CONFIG;

	const STALE_MS = 15000;

	let wsStatus = 'idle';
	let temperature = null;
	let humidity = null;
	let unit = null;
	let lastUpdate = null;
	let lastPayload = null;
	let now = Date.now();
	let socket;
	let errorMessage = '';

	const wsLabels = {
		idle: 'Idle',
		connecting: 'Connecting',
		connected: 'Connected',
		closed: 'Closed',
		error: 'Error'
	};

	function parseJsonMaybe(value) {
		if (value && typeof value === 'object') return value;
		if (typeof value !== 'string') return null;
		try {
			return JSON.parse(value);
		} catch {
			return null;
		}
	}

	function toNumber(value) {
		const numberValue = typeof value === 'string' ? Number(value) : value;
		return Number.isFinite(numberValue) ? numberValue : null;
	}

	function handleMessage(raw) {
		const msg = parseJsonMaybe(raw);
		if (!msg || typeof msg !== 'object') return;

		// Le bridge envoie { topic, payload }
		if (msg.topic !== MQTT_TOPIC) return;

		const payload = parseJsonMaybe(msg.payload);
		if (!payload || typeof payload !== 'object') return;

		const nextTemp = toNumber(payload.temp ?? payload.temperature ?? payload.tempC ?? payload.temp_c);
		const nextHum = toNumber(payload.humidity ?? payload.hum ?? payload.humPct ?? payload.humidityPct);
		const nextUnit = typeof payload.unit === 'string' ? payload.unit : unit;

		if (nextTemp !== null) temperature = nextTemp;
		if (nextHum !== null) humidity = nextHum;
		if (nextUnit) unit = nextUnit;

		lastPayload = payload;
		lastUpdate = Date.now();
	}

	function connect() {
		if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
			return;
		}

		wsStatus = 'connecting';
		errorMessage = '';

		socket = new WebSocket(WS_URL);

		socket.onopen = () => {
			wsStatus = 'connected';
		};

		socket.onerror = () => {
			wsStatus = 'error';
			errorMessage = 'WebSocket error';
		};

		socket.onclose = () => {
			wsStatus = 'closed';
		};

		socket.onmessage = (event) => {
			handleMessage(event.data);
		};
	}

	// Envoie une commande a l'ESP32 via le bridge
	function sendUnit(nextUnit) {
		if (!socket || socket.readyState !== WebSocket.OPEN) {
			errorMessage = 'WebSocket not connected';
			return;
		}

		const message = {
			topic: COMMAND_TOPIC,
			payload: { unit: nextUnit }
		};

		socket.send(JSON.stringify(message));
	}

	function formatNumber(value, digits = 1) {
		return Number.isFinite(value) ? value.toFixed(digits) : 'n/a';
	}

	function since(ts) {
		if (!ts) return 'n/a';
		const diff = Math.max(0, Math.round((now - ts) / 1000));
		return diff < 60 ? `${diff}s` : `${Math.floor(diff / 60)}m ${diff % 60}s`;
	}

	$: isOnline = lastUpdate ? now - lastUpdate < STALE_MS : false;

	onMount(() => {
		connect();
		const timer = setInterval(() => {
			now = Date.now();
		}, 1000);
		return () => {
			if (socket) socket.close();
			clearInterval(timer);
		};
	});
</script>

<svelte:head>
	<title>Station Meteo - GP5</title>
</svelte:head>

<main class="relative min-h-screen overflow-hidden px-6 py-12 text-slate-900">
	<div
		aria-hidden="true"
		class="pointer-events-none absolute -top-24 right-[-15%] h-72 w-72 rounded-full bg-[radial-gradient(circle_at_top,_rgba(14,165,233,0.35),_rgba(45,212,191,0))] blur-3xl"
	></div>
	<div
		aria-hidden="true"
		class="pointer-events-none absolute -bottom-28 left-[-10%] h-96 w-96 rounded-full bg-[radial-gradient(circle_at_bottom,_rgba(251,146,60,0.35),_rgba(251,146,60,0))] blur-3xl"
	></div>

	<div class="mx-auto flex max-w-5xl flex-col gap-8">
		<header class="flex flex-wrap items-end justify-between gap-6">
			<div class="space-y-2">
				<p
					class="inline-flex w-fit items-center gap-2 rounded-full border border-white/70 bg-white/70 px-4 py-1 text-xs uppercase tracking-[0.3em] text-teal-700 shadow-sm"
				>
					<span class="h-2 w-2 rounded-full bg-teal-500"></span>
					GP5 meteo live
				</p>
				<h1 class="font-display text-4xl leading-tight md:text-5xl">Station Meteo</h1>
				<p class="max-w-2xl text-lg text-slate-700">
					Temperature et humidite en temps reel via MQTT + WebSocket.
				</p>
			</div>
			<button
				class="inline-flex items-center gap-2 rounded-2xl bg-teal-600 px-4 py-2 text-xs font-semibold uppercase tracking-[0.2em] text-white shadow-[0_16px_30px_-16px_rgba(13,148,136,0.9)] transition hover:-translate-y-0.5 hover:bg-teal-500 active:translate-y-0"
				on:click={connect}
			>
				Reconnect
			</button>
		</header>

		<section class="grid gap-6 lg:grid-cols-[1.2fr_0.8fr]">
			<div
				class="rounded-3xl border border-white/70 bg-white/80 p-6 shadow-[0_30px_80px_-50px_rgba(15,23,42,0.45)] backdrop-blur"
			>
				<div class="flex flex-wrap items-center justify-between gap-4">
					<div class="space-y-2">
						<p class="text-xs uppercase tracking-[0.2em] text-slate-400">Lecture en cours</p>
						<h2 class="font-display text-3xl">GP5-weather</h2>
					</div>
					<span
						class={`inline-flex items-center gap-2 rounded-full border px-3 py-1 text-xs font-semibold ${
							isOnline
								? 'border-emerald-200 bg-emerald-100 text-emerald-700'
								: 'border-rose-200 bg-rose-100 text-rose-700'
						}`}
					>
						<span class={`h-2 w-2 rounded-full ${isOnline ? 'bg-emerald-500' : 'bg-rose-500'}`}></span>
						{isOnline ? 'Online' : 'Offline'}
					</span>
				</div>

				<div class="mt-6 grid gap-4 sm:grid-cols-3">
					<div class="rounded-2xl border border-slate-200/70 bg-white p-4">
						<p class="text-xs uppercase tracking-[0.2em] text-slate-400">Temperature</p>
						<p class="mt-2 text-3xl font-semibold text-slate-700">
							{formatNumber(temperature, 1)} {unit ?? 'C'}
						</p>
					</div>
					<div class="rounded-2xl border border-slate-200/70 bg-white p-4">
						<p class="text-xs uppercase tracking-[0.2em] text-slate-400">Humidite</p>
						<p class="mt-2 text-3xl font-semibold text-slate-700">{formatNumber(humidity, 0)} %</p>
					</div>
					<div class="rounded-2xl border border-slate-200/70 bg-white p-4">
						<p class="text-xs uppercase tracking-[0.2em] text-slate-400">Derniere maj</p>
						<p class="mt-2 text-3xl font-semibold text-slate-700">{since(lastUpdate)}</p>
					</div>
				</div>

				<div class="mt-6 rounded-2xl border border-slate-200/70 bg-white p-4 text-sm text-slate-600">
					<p class="text-xs uppercase tracking-[0.2em] text-slate-400">Bridge</p>
					<p class="mt-2">WebSocket: {WS_URL}</p>
					<p class="mt-1">Status: {wsLabels[wsStatus] ?? wsStatus}</p>
					<p class="mt-1">MQTT topic: {MQTT_TOPIC}</p>
					{#if errorMessage}
						<p class="mt-2 text-rose-600">{errorMessage}</p>
					{/if}
				</div>
			</div>

			<aside
				class="flex flex-col gap-4 rounded-3xl border border-white/70 bg-white/75 p-6 shadow-[0_20px_60px_-40px_rgba(15,23,42,0.4)] backdrop-blur"
			>
				<div>
					<p class="text-xs uppercase tracking-[0.25em] text-slate-400">Bonus</p>
					<h3 class="font-display mt-2 text-xl">Changer l'unite</h3>
				</div>
				<p class="text-sm text-slate-600">
					Le bouton envoie une commande via le bridge : <code>{"{ topic, payload: { unit } }"}</code>
				</p>
				<div class="grid gap-3 sm:grid-cols-2">
					<button
						class="rounded-2xl border border-slate-200 bg-white px-4 py-3 text-sm font-semibold text-slate-700 transition hover:-translate-y-0.5 hover:border-teal-300 hover:text-teal-700"
						on:click={() => sendUnit('C')}
					>
						Passer en C
					</button>
					<button
						class="rounded-2xl border border-slate-200 bg-white px-4 py-3 text-sm font-semibold text-slate-700 transition hover:-translate-y-0.5 hover:border-teal-300 hover:text-teal-700"
						on:click={() => sendUnit('F')}
					>
						Passer en F
					</button>
				</div>

				<div class="rounded-2xl border border-slate-200/70 bg-white p-4 text-sm text-slate-600">
					<p class="text-xs uppercase tracking-[0.2em] text-slate-400">Dernier payload</p>
					<pre
						class="mt-2 max-h-40 overflow-auto whitespace-pre-wrap break-words rounded-xl border border-slate-200/70 bg-slate-50/70 p-3 text-[11px] leading-relaxed text-slate-700"
					>{lastPayload ? JSON.stringify(lastPayload, null, 2) : 'n/a'}</pre>
				</div>
			</aside>
		</section>
	</div>
</main>