ytree.exe

Compilare il codice
1)Inserire la cartella src cosi come presentata dentro la Yocto/GL;
2)Aprire il file CMakeList.txt presente alla radice di Yocto/GL e scendere fino alla riga 37,
	inserire il seguente comando: add_subdirectory(src);
3) compilare il codice con cmake;

Setup delle risorse necessarie:
1)Nella cartella [yoctoRoot]/tests inserire la cartella node_crown con le risorse necessarie per produrre l'albero

comandi da shell:

per eseguire il codice se su windows, posizionarsi all'interno di [yoctoRoot] ed eseguire il seguenti comando:
	.\bin\ytree_multy.exe --scene "C:\yocto-AlberoProcedurale\tests\tests_assets\node_crown\node_crown_test.json" --interactive --random --numattr 2000 --attr_range 0.5 --kill_range 0.46 --depth 200 --length 0.1
	
Sono state aggiunte delle opzioni:

	--random: per generare un insieme di attraction points randomico, se non presente la funzione relativa alla generazione casuale impieghera' sempre il medesimo seed per la creazione dei punti: 58380
	
	--numattr: definisce il numero di attraction points da generare
	--attr_range: definisce il raggio di attrazione per i rami
	--kill_range: definisce il raggio di eliminazione degli influence points 
	--depth: definisce la profondita' massima dell'albero.
	--lenght: definisce la lunghezza di un singolo ramo.
	
N.B.: Modificare i valori per avere risultati differenti, quelli presenti nell'esempio sono quelli che crediamo 
essere i più realistici.