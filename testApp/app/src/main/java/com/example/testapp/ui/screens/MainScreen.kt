package com.example.testapp.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.testapp.data.Game
import com.example.testapp.data.User
import com.example.testapp.data.Score
import com.example.testapp.ui.components.CommonHeader
import com.example.testapp.ui.components.CommonScreenLayout
import java.text.SimpleDateFormat
import java.util.*

/**
 * メイン画面（ユーザー一覧とゲーム履歴）
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    users: List<User>,
    gameHistory: List<Triple<Game, List<Pair<User, Score?>>, Int>>,
    onAddGameScore: () -> Unit,
    onDeleteGame: (String) -> Unit,
    onEditScore: (String, Int) -> Unit,
    onViewHistory: () -> Unit,
    onResetApp: () -> Unit,
    onBackToSelect: (() -> Unit)? = null,
    modifier: Modifier = Modifier
) {
    var showResetDialog by remember { mutableStateOf(false) }
    
    Scaffold(
        floatingActionButton = {
            FloatingActionButton(
                onClick = onAddGameScore,
                containerColor = MaterialTheme.colorScheme.primary
            ) {
                Icon(
                    imageVector = Icons.Default.Add,
                    contentDescription = "新しい得点を追加",
                    tint = MaterialTheme.colorScheme.onPrimary
                )
            }
        }
    ) { paddingValues ->
        CommonScreenLayout(
            modifier = modifier.padding(paddingValues)
        ) {
            // ヘッダー
            CommonHeader(
                title = "得点集計",
                subtitle = "${users.size}人参加中"
            )
            
            Spacer(modifier = Modifier.height(16.dp))
            
            // アクションボタン
            Row {
                onBackToSelect?.let { backAction ->
                    IconButton(onClick = backAction) {
                        Icon(
                            imageVector = Icons.Default.ArrowBack,
                            contentDescription = "セッション選択に戻る",
                            tint = MaterialTheme.colorScheme.onPrimaryContainer
                        )
                    }
                }
                IconButton(onClick = onViewHistory) {
                    Icon(
                        imageVector = Icons.Default.History,
                        contentDescription = "全履歴を見る",
                        tint = MaterialTheme.colorScheme.onPrimaryContainer
                    )
                }
                IconButton(onClick = { showResetDialog = true }) {
                    Icon(
                        imageVector = Icons.Default.Refresh,
                        contentDescription = "リセット",
                        tint = MaterialTheme.colorScheme.onPrimaryContainer
                    )
                }
            }
            
            // ユーザー一覧表示
            Card(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp),
                elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
            ) {
                Column(
                    modifier = Modifier.padding(16.dp)
                ) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            text = "現在の順位",
                            fontSize = 18.sp,
                            fontWeight = FontWeight.Medium,
                            color = MaterialTheme.colorScheme.primary
                        )
                        Text(
                            text = "合計得点",
                            fontSize = 14.sp,
                            fontWeight = FontWeight.Medium,
                            color = MaterialTheme.colorScheme.primary
                        )
                    }
                    
                    Spacer(modifier = Modifier.height(16.dp))
                    
                    if (users.isNotEmpty()) {
                        // ユーザー別の合計得点を計算して順位表示
                        val userScores = users.map { user ->
                            val totalScore = gameHistory.flatMap { (_, userScores, _) ->
                                userScores.filter { it.first.id == user.id }
                                    .mapNotNull { it.second?.value ?: 0 }
                            }.sum()
                            user to totalScore
                        }.sortedByDescending { it.second }
                        
                        userScores.forEachIndexed { index, (user, totalScore) ->
                            Surface(
                                modifier = Modifier.fillMaxWidth(),
                                color = if (index == 0) {
                                    MaterialTheme.colorScheme.primaryContainer.copy(alpha = 0.3f)
                                } else {
                                    MaterialTheme.colorScheme.surface
                                },
                                onClick = {
                                    // ユーザーのスコア詳細画面に遷移する場合はここに処理を追加
                                }
                            ) {
                                Row(
                                    modifier = Modifier
                                        .fillMaxWidth()
                                        .padding(vertical = 8.dp, horizontal = 8.dp),
                                    horizontalArrangement = Arrangement.SpaceBetween,
                                    verticalAlignment = Alignment.CenterVertically
                                ) {
                                    Row(
                                        verticalAlignment = Alignment.CenterVertically
                                    ) {
                                        Text(
                                            text = "${index + 1}位",
                                            fontWeight = FontWeight.Bold,
                                            color = if (index == 0) MaterialTheme.colorScheme.primary else MaterialTheme.colorScheme.onSurface
                                        )
                                        Spacer(modifier = Modifier.width(12.dp))
                                        Text(
                                            text = user.name,
                                            fontSize = 16.sp,
                                            color = MaterialTheme.colorScheme.onSurface
                                        )
                                    }
                                    Text(
                                        text = "${totalScore}点",
                                        fontSize = 16.sp,
                                        fontWeight = FontWeight.Bold,
                                        color = MaterialTheme.colorScheme.primary
                                    )
                                }
                            }
                            if (index < userScores.size - 1) {
                                HorizontalDivider()
                            }
                        }
                    } else {
                        Text(
                            text = "参加者がいません",
                            color = MaterialTheme.colorScheme.onSurfaceVariant
                        )
                    }
                }
            }
            
            Spacer(modifier = Modifier.height(24.dp))
            
            // ゲーム履歴
            if (gameHistory.isNotEmpty()) {
                Text(
                    text = "ゲーム履歴",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Medium,
                    color = MaterialTheme.colorScheme.onSurface,
                    modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp)
                )
                
                LazyColumn(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(horizontal = 16.dp),
                    verticalArrangement = Arrangement.spacedBy(12.dp),
                    contentPadding = PaddingValues(bottom = 16.dp)
                ) {
                    items(gameHistory) { (game, userScores, gameTotal) ->
                        GameHistoryCard(
                            game = game,
                            userScores = userScores,
                            gameTotal = gameTotal,
                            onDeleteGame = onDeleteGame,
                            onEditScore = onEditScore
                        )
                    }
                }
            } else {
                // ゲーム履歴がない場合
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(24.dp),
                    verticalArrangement = Arrangement.Center,
                    horizontalAlignment = Alignment.CenterHorizontally
                ) {
                    Icon(
                        imageVector = Icons.Default.SportsEsports,
                        contentDescription = null,
                        modifier = Modifier.size(64.dp),
                        tint = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Text(
                        text = "まだゲームが開始されていません",
                        fontSize = 18.sp,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Text(
                        text = "「ゲームスコアを追加」ボタンでスコアを記録しましょう",
                        fontSize = 14.sp,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }
        }
        
        // リセット確認ダイアログ
        if (showResetDialog) {
            AlertDialog(
                onDismissRequest = { showResetDialog = false },
                title = { Text("アプリをリセット") },
                text = { Text("全てのデータが削除されます。最初からやり直しますか？") },
                confirmButton = {
                    TextButton(
                        onClick = {
                            onResetApp()
                            showResetDialog = false
                        }
                    ) {
                        Text("リセット")
                    }
                },
                dismissButton = {
                    TextButton(onClick = { showResetDialog = false }) {
                        Text("キャンセル")
                    }
                }
            )
        }
    }
}

@Composable
private fun GameHistoryCard(
    game: Game,
    userScores: List<Pair<User, Score?>>,
    gameTotal: Int,
    onDeleteGame: (String) -> Unit,
    onEditScore: (String, Int) -> Unit,
    modifier: Modifier = Modifier
) {
    var isExpanded by remember { mutableStateOf(false) }
    var showDeleteDialog by remember { mutableStateOf(false) }
    var showEditDialog by remember { mutableStateOf(false) }
    var editingScore by remember { mutableStateOf<Score?>(null) }
    var editingUser by remember { mutableStateOf<User?>(null) }
    var editScoreText by remember { mutableStateOf("") }
    val dateFormat = remember { SimpleDateFormat("MM/dd HH:mm", Locale.getDefault()) }
    
    Card(
        modifier = modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            // ゲーム情報ヘッダー
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(bottom = 8.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text(
                        text = game.name,
                        fontSize = 16.sp,
                        fontWeight = FontWeight.Bold,
                        color = MaterialTheme.colorScheme.onSurface
                    )
                    Text(
                        text = dateFormat.format(Date(game.timestamp)),
                        fontSize = 12.sp,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
                
                Row(
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "合計: ${gameTotal}点",
                        fontSize = 14.sp,
                        fontWeight = FontWeight.Medium,
                        color = MaterialTheme.colorScheme.primary
                    )
                    Spacer(modifier = Modifier.width(8.dp))
                    Row {
                        IconButton(
                            onClick = { isExpanded = !isExpanded },
                            modifier = Modifier.size(32.dp)
                        ) {
                            Icon(
                                imageVector = if (isExpanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                                contentDescription = if (isExpanded) "閉じる" else "展開",
                                tint = MaterialTheme.colorScheme.onSurfaceVariant
                            )
                        }
                        IconButton(
                            onClick = { showDeleteDialog = true },
                            modifier = Modifier.size(32.dp)
                        ) {
                            Icon(
                                imageVector = Icons.Default.Delete,
                                contentDescription = "削除",
                                tint = MaterialTheme.colorScheme.error
                            )
                        }
                    }
                }
            }
            
            // 展開時の詳細表示
            if (isExpanded) {
                HorizontalDivider()
                Spacer(modifier = Modifier.height(8.dp))
                
                userScores.forEach { (user, score) ->
                    Row(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(vertical = 4.dp),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            text = user.name,
                            fontSize = 14.sp,
                            color = MaterialTheme.colorScheme.onSurface
                        )
                        Row(
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Text(
                                text = "${score?.value ?: 0}点",
                                fontSize = 14.sp,
                                fontWeight = FontWeight.Medium,
                                color = MaterialTheme.colorScheme.primary
                            )
                            Spacer(modifier = Modifier.width(8.dp))
                            IconButton(
                                onClick = { 
                                    score?.let { s ->
                                        editingScore = s
                                        editingUser = user
                                        editScoreText = s.value.toString()
                                        showEditDialog = true
                                    }
                                },
                                modifier = Modifier.size(24.dp)
                            ) {
                                Icon(
                                    imageVector = Icons.Default.Edit,
                                    contentDescription = "編集",
                                    tint = MaterialTheme.colorScheme.onSurfaceVariant
                                )
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 削除確認ダイアログ
    if (showDeleteDialog) {
        AlertDialog(
            onDismissRequest = { showDeleteDialog = false },
            title = { Text("ゲームを削除") },
            text = { Text("「${game.name}」を削除しますか？この操作は取り消せません。") },
            confirmButton = {
                TextButton(
                    onClick = {
                        onDeleteGame(game.id)
                        showDeleteDialog = false
                    }
                ) {
                    Text("削除", color = MaterialTheme.colorScheme.error)
                }
            },
            dismissButton = {
                TextButton(onClick = { showDeleteDialog = false }) {
                    Text("キャンセル")
                }
            }
        )
    }
    
    // スコア編集ダイアログ
    if (showEditDialog && editingScore != null && editingUser != null) {
        AlertDialog(
            onDismissRequest = { 
                showEditDialog = false
                editingScore = null
                editingUser = null
            },
            title = { Text("スコアを編集") },
            text = {
                Column {
                    Text("${editingUser!!.name}さんのスコアを編集")
                    Spacer(modifier = Modifier.height(8.dp))
                    OutlinedTextField(
                        value = editScoreText,
                        onValueChange = { editScoreText = it },
                        label = { Text("得点") },
                        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                        singleLine = true
                    )
                }
            },
            confirmButton = {
                TextButton(
                    onClick = {
                        val newScore = editScoreText.toIntOrNull()
                        if (newScore != null && editingScore != null) {
                            onEditScore(editingScore!!.id, newScore)
                        }
                        showEditDialog = false
                        editingScore = null
                        editingUser = null
                    },
                    enabled = editScoreText.toIntOrNull() != null
                ) {
                    Text("保存")
                }
            },
            dismissButton = {
                TextButton(onClick = { 
                    showEditDialog = false
                    editingScore = null
                    editingUser = null
                }) {
                    Text("キャンセル")
                }
            }
        )
    }
}
