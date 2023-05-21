Compilation 
	make , make all -> compile les programmes client proxy server
	make client -> compile le programme client
	make server -> compile le programme server
	make proxy -> compile le programme proxy

Execution :
	

Utilisation : 
	<arg> -> argument obligatoire
	(arg) -> argument optionnel

client <adresse proxy> (port proxy) (port client) :
	Lance un programme client qui se connecte au proxy en utilisant le protocole TCP
	Si le programme n'arrive pas à se connecter, il rééssaie MAX_ATTEMPTS fois
	Avec un délai de 1s

	On demande en option le port d'écoute du proxy et le port par lequel
	notre client et le proxy divent pouvoir communiquer.
	S'il ne sont pas fournis, on utilise par défaut  
	DEFAULT_LISTEN_PORT_PROXY pour le port du proxy 
	et DEFAULT_CLIENT_PORT pour notre client.

	On commence par s'authentifier en envoyant un paquet de type
	DT_CON permmettant au proxy de se préparer à recevoir le nom du client.
	On envoie ensuite un pseudo.
	Si le nom du client existe déjà, le proxy devrait envoyer DT_AEU.
	Par gain de temps et de simplicité, ça n'a finalement pas été implémenté.
	On peut donc avoir plusieurs utilisateurs connectés avec le même nom.
	
	On demande ensuite à l'utilisateur le nom d'un client ou l'adresse ip
	d'un client avec qui il voudrait communiquer.
	On commence ensuite le chat en envoyant DT_BOC au proxy.
	Dans l'idée, celui-ci devra l'aiguiller vers l'autre client 
	pour lui indiquer qu'un chat va pouvoir débuter.
	Ensuite, on échange des paquets de type DT_CEX contenant un caractère
	envoyé par un client.
	A la suite de la réception d'un paquet DT_CLO ou DT_EOC de la part d'un autre
	client, on arrête la communication.

	On devrait avoir un fonctionnement similaire pour un échange de fichier 
	avec  DT_BOF, DT_FEX, DT_EOF mais cela demande d'autres fonctions d'interactions 
	avec l'utilisateur pour sauver le fichier. L'implémentation était prévue mais n'a pas aboutie
	par gain de temps.

	Les échanges et retransmission fonctionnent un peu à la façon tcp, le serveur acquitte chaque paquet
	non corrompu. Quand on recoit un aquittement, on déplace la fenetre de renvoi des données.
	Si on recoit un acquittement négatif on si on dépasse un temps d'attente 
	avant reception de l'acquittement du dernier paquet envoyé, on renvoit les données de la fenêtre courante.
	Si on dépasse un certain nombres de tentatives de renvois, on laisse tomber, on laisse à l'utilsateur
	la possibilité d'envoyé un nouveau message, on lui signale que toutes les données n'ont peut-être pa sété envoyées.
	Dans le cas d'un échange de fichier, on laisse carrément tomber l'échane de fichier. On laissera à l'utilisateur le choix
	relancer le protocole d'échange ou non.


serveur  (port ecoute)  :
	Prend éventuellement en entrée, le port sur lequel écouter.
	Sinon, on écoute par défaut sur le port DEFAULT_EXCHANGE_PORT_SERVER.

	Le serveur a un fonctionnement assez simple. Il était initialement multi-threadé 
	et bufferisé mais il a été simplifié au maximum pour facililter le debuggage et la compréhension 
	de sa conception. 
	Il se contente donc dans un premier temps d'attendre une unique entité (le proxy), et accepte la connexion.
	
	Le serveur se contente de vérifier l'intégrité des données dans le paquet, corriger si possible.
	Il envoie un acquittement au client ayant envoyé le paquet si les données son intègres ou corrigées et
	Il aiguille le paquet vers le destinataire en renvoyant le paquet vers le proxy.
	Si les données ne sont pas intègres, il envoie simplement un paquet de type DT_NAK à l'envoyeur.

	Note1 : chaque paquet entre via le proxy et chaque paque sortant est envoyé à nouveau vers celui-ci.

proxy <liste des serveurs> (port_ecoute) :

	Le proxy prend en entrée un fichier contenant une liste de serveur sous le format:
		xxx.xxx.xxx.xxx:port_ecoute_serveur1
		xxx.xxx.xxx.xxx:port_ecoute_serveur2
	ex: 
		8.8.8.8:80
		1.1.1.1:53
		..

	Note : 
	Le proxy est configuré de sorte à pouvoir se connecter à plusieurs serveurs, chaque serveur ayant une charge
	Dès qu'un client se connecte, on lui attribue pour le traitement de ses échanges, le serveur avec la charge la moins élèvée à son arrivée.

	Le proxy prend en option un port d'écoute, sinon il vaut par défaut DEFAULT_LISTEN_PORT_PROXY.

	Le proxy introduit pour chaque paquet de données d'échange de type DT_CEX ou DT_FEX, un bruit
	pour éventuellement corrompre les données.
	Il effectue également un filtrage. Il garde en mémoire les clients en conservant le pseudo du client,
	le serveur par lequel transite ses envois. Notamment, quand un client indique qu'il veut arrêter un 
	echange de messages, de fichier ou mettre fin à la connection, le proxy indique directement au client destinatire
	d'arrêter également l'échange sans passer par le serveur.

